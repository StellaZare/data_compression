#include "output_stream.hpp"
#include "LZSSEncoder.hpp"
#include "prefix_codes.hpp"
#include <cassert>

// To compute CRC32 values, we can use this library
// from https://github.com/d-bahr/CRCpp
#define CRCPP_USE_CPP11
#include "CRC.h"

static const u32 b2_size {(1<<16)-1};
static const u32 LL_size {288};
static const u32 distance_size {30};
static const u32 CL_size {19};

class TypeTwoEncoder{
    public:
    TypeTwoEncoder () {}

    void Encode(OutputBitStream& stream, u32 block_size, std::array <u8, b2_size>& block_contents, bool is_last){

        // Encode block w LZ SS
        LZSSEncoder_2 lzss {};
        const std::vector<Code>& bit_stream = lzss.Encode(block_size, block_contents);

        // generate as least 2 distance codes
        incrementDistanceCount(1);
        incrementDistanceCount(2);
        
        // get symbol counts
        for(const Code& curr : bit_stream){
            if(curr.code_type == 1){
                incrementLiteralCount(curr.value);
                ++LL_total_count;   
            }
            else if (curr.code_type == 2){
                incrementLengthCount(curr.value);
                ++LL_total_count;
            }
            else if (curr.code_type == 3){
                incrementDistanceCount(curr.value);
                ++distance_total_count; 
            }
            else{
                std::cerr << "invalid code type in bit_stream" << std::endl;
            }
            // count EOB 
            LL_count.at(256) = 1;
        }
        //printLLCount();
        //printDistanceCount();

        // get symbol probabilities
        std::vector < std::pair <std::vector<u32>, double> > LL_probability {};
        std::vector < std::pair <std::vector<u32>, double> > distance_probability {};
        getLLProbabilities(LL_probability);
        getDistanceProbabilities(distance_probability);
        // printProbabilities();

        // get code lengths
        PackageMerge pm {};
        std::vector<u32> LL_code_lengths {pm.getSymbolLengths(LL_probability, LL_size)};
        std::vector<u32> distance_code_lengths {pm.getSymbolLengths(distance_probability, distance_size)};

        // get encoding
        DynamicCodes LL_codes {LL_code_lengths};
        DynamicCodes distance_codes {distance_code_lengths};

        // use the code lengths to count CL
        getLLDistanceEncoding(LL_code_lengths, distance_code_lengths);
        std::vector < std::pair <std::vector<u32>, double> > CL_probability {};
        getCLProbabilities(CL_probability);
        std::vector<u32> CL_code_lengths {pm.getSymbolLengths(CL_probability, CL_size)};
        DynamicCodes CL_codes {CL_code_lengths};
        printCLCount();
        printLLDistanceEncoding();

        pushBlockHeader(stream, is_last, CL_code_lengths);
        pushLLDistanceLengths(stream, CL_codes);
        pushBitStream(stream, bit_stream, LL_codes, distance_codes );
        pushEOB(stream, LL_codes);
    }

    private:
    LLCodesBlock_1 fixed_LL_codes {};
    DistanceCodesBlock_1 fixed_distance_codes {};
    
    std::array <u32, LL_size> LL_count {};       // LL_count[symbol] = count of that symbol
    std::array <u32, distance_size> distance_count {};  // distance_count[symbol] = count of that
    double LL_total_count {};
    double distance_total_count {};

    std::array <u32, CL_size> CL_count {};
    double CL_total_count {};
    std::vector <u32> LL_distance_encoding {};


    void pushBlockHeader(OutputBitStream& stream, bool& is_last, const std::vector<u32>& CL_code_lengths){
        stream.push_bit(is_last); 
        stream.push_bits(2, 2);                 // 2 bit block type
        stream.push_bits(LL_size-257, 5);       // 5 bit HLIT
        stream.push_bits(distance_size-1 , 5);  // 5 bit HDIST
        stream.push_bits(CL_size-4, 4);         // 4 bit HCLEN

        std::array <u32, CL_size> order {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
        for(u32 idx = 0; idx < CL_size; ++idx){
            u8 length = CL_code_lengths.at(order.at(idx));
            assert(length <= 7);
            stream.push_bits(length, 3);        // 3 bit CL lengths  
        }
    }

    void pushLLDistanceLengths(OutputBitStream& stream, DynamicCodes& CL_codes){
        u32 idx = 0;
        while(idx < LL_distance_encoding.size()){
            pushSymbol(stream, CL_codes.getCodeSequence(LL_distance_encoding.at(idx)));
            if(LL_distance_encoding.at(idx) == 16){
                stream.push_bits(LL_distance_encoding.at(++idx), 2);
            }
            else if(LL_distance_encoding.at(idx) == 17){
                stream.push_bits(LL_distance_encoding.at(++idx), 3);
            }
            else if(LL_distance_encoding.at(idx) == 18){
                stream.push_bits(LL_distance_encoding.at(++idx), 7);
            }
            ++idx;
        }
    }
    void pushSymbol(OutputBitStream& stream, const std::vector<bool>& seq){
        for(u32 bit : seq){
            stream.push_bit(bit);
        }
    }

    void pushBitStream(OutputBitStream& stream, const std::vector<Code>& bit_stream, DynamicCodes& LL_codes, DynamicCodes& distance_codes ){
        for(const Code& curr : bit_stream){
            if (curr.code_type == 1){
                pushSymbol(stream, LL_codes.getCodeSequence(curr.value));
            }
            else if (curr.code_type == 2){
                u32 length = curr.value;
                u32 length_symbol = fixed_LL_codes.getLengthSymbol(length);
                u32 base_length = fixed_LL_codes.getBaseLength(length);
                u32 length_offset = fixed_LL_codes.getLengthOffset(length_symbol);
                pushSymbol(stream, LL_codes.getCodeSequence(length_symbol));
                stream.push_bits(length - base_length, length_offset);
            }
            else if (curr.code_type == 3){
                u32 distance = curr.value;
                u32 distance_symbol = fixed_distance_codes.getDistanceSymbol(distance);
                u32 base_distance = fixed_distance_codes.getBaseDistance(distance);
                u32 distance_offset = fixed_distance_codes.getNumOffset(distance_symbol);
                pushSymbol(stream, distance_codes.getCodeSequence(distance_symbol));
                stream.push_bits(distance - base_distance, distance_offset);
            }
            else{
                std::cerr << "invalid code type in bit_stream" << std::endl;
            }
        }
    }

    void pushEOB(OutputBitStream& stream, DynamicCodes& LL_codes){
        std::vector<bool> EOB = LL_codes.getCodeSequence(256);
        for(auto bit : EOB){
            stream.push_bit(bit);
        }
    }
    
    void incrementLiteralCount(u32 literal){
        ++LL_count.at(literal);
    }
    void incrementLengthCount(u32 length){
        u32 symbol = fixed_LL_codes.getLengthSymbol(length);
        ++LL_count.at(symbol);
    }
    void incrementDistanceCount(u32 dist){
        u32 symbol = fixed_distance_codes.getDistanceSymbol(dist);
        ++distance_count.at(symbol);
    }
    void printLLCount(){
        for(u32 idx = 0; idx < LL_size; ++idx){
            std::cerr << "[" << idx << ", " << LL_count.at(idx) << "]"<< std::endl;
        }
    }
    void printDistanceCount(){
        for(u32 idx = 0; idx < distance_size; ++idx){
            std::cerr << "[" << idx << ", " << distance_count.at(idx) << "]"<< std::endl;
        }
    }

    void getLLProbabilities(std::vector < std::pair <std::vector<u32>, double> >& LL_probability){
        for(u32 idx = 0; idx < LL_size; ++idx){
            if(LL_count.at(idx) != 0){
                double prob = LL_count.at(idx) / LL_total_count;
                LL_probability.push_back({{idx}, prob});
            }
        }
    }
    void getDistanceProbabilities(std::vector < std::pair <std::vector<u32>, double> >& distance_probability){
        for(u32 idx = 0; idx < distance_size; ++idx){
            if(distance_count.at(idx) != 0){
                double prob = distance_count.at(idx) / distance_total_count;
                distance_probability.push_back({{idx}, prob});
            }
        }
    }
    // void printProbabilities(std::vector < std::pair <std::vector<u32>, double> >& LL_probability, std::vector < std::pair <std::vector<u32>, double> >& distance_probability){
    //     for(u32 idx = 0; idx < LL_proabbilities.size(); ++idx){
    //         std::cerr << LL_probabilities.first.at(0) << " " << LL_probabilities.second << std::endl;
    //     }

    // }
    void getCLProbabilities(std::vector < std::pair <std::vector<u32>, double> >& CL_probability){
        for(u32 idx = 0; idx < CL_size; ++idx){
            if(CL_count.at(idx) != 0){
                double prob = CL_count.at(idx) / CL_total_count;
                CL_probability.push_back({{idx}, prob});
            }
        }
    }

    void getLLDistanceEncoding(std::vector<u32>& LL_code_lengths, std::vector<u32>& distance_code_lengths){
        std::vector <u32> ll_and_distance {};
        ll_and_distance.insert(ll_and_distance.end(), LL_code_lengths.begin(), LL_code_lengths.end());
        ll_and_distance.insert(ll_and_distance.end(), distance_code_lengths.begin(), distance_code_lengths.end());

        // for(u32& v : ll_and_distance){
        //     std::cerr << v << std::endl;
        // }

        u32 idx = 0;
        while(idx < ll_and_distance.size()){
            u32 run_length = getRunLength(ll_and_distance, idx);
            // std::cerr << "idx " << idx << " " << ll_and_distance.at(idx) << " RLE " << run_length;

            if (ll_and_distance.at(idx) == 0 && run_length > 10){
                // std::cerr << " case 18" << std::endl;

                run_length = std::min(run_length, u32(127+11));
                ++CL_count.at(18);       // increment RLE
                ++CL_total_count;
                // LL_distance_encoding << symbol << RLE << len
                LL_distance_encoding.push_back(18);
                LL_distance_encoding.push_back(run_length-11);
                idx += run_length;
            }
            else if (ll_and_distance.at(idx) == 0 && run_length >= 3){
                // std::cerr << " case 17" << std::endl;
                ++CL_count.at(17);       // increment RLE
                ++CL_total_count;
                // LL_distance_encoding << symbol << RLE << len
                LL_distance_encoding.push_back(17);
                LL_distance_encoding.push_back(run_length-3);
                idx += run_length;

            }else if (run_length >= 3){    
                // std::cerr << " case 16" << std::endl; 
                run_length = std::min(run_length, u32(3+3));
                ++CL_count.at(ll_and_distance.at(idx));        // increment symbol
                ++CL_count.at(16);       // increment RLE
                CL_total_count += 2;
                // LL_distance_encoding << symbol << RLE << len
                LL_distance_encoding.push_back(ll_and_distance.at(idx));
                LL_distance_encoding.push_back(16);
                LL_distance_encoding.push_back(run_length-3);
                idx += (run_length+1);
            }else{
                // std::cerr << " case reg" << std::endl;
                ++CL_count.at(ll_and_distance.at(idx));        // increment symbol
                ++CL_total_count;
                LL_distance_encoding.push_back(ll_and_distance.at(idx));
                ++idx;
            }    
        }
    }

    u32 getRunLength(std::vector<u32>& ll_and_distance, u32 start){
        u32 idx = start;
        while(idx < ll_and_distance.size() && ll_and_distance.at(idx) == ll_and_distance.at(start)){
            ++idx;
        }
        if(ll_and_distance.at(start) != 0)
            --idx;
        return idx-start;
    }

    void printLLDistanceEncoding(){
        std::cerr << "Encoding: " << std::endl;
        for(u32& value : LL_distance_encoding){
            std::cerr << value << " ";
        }
        std::cerr << std::endl;
    }

    void printCLCount(){
        std::cerr << "CL count table: " << std::endl;
        for(u32 idx = 0; idx < CL_count.size(); ++idx){
            std::cerr << idx << " " << CL_count.at(idx) << std::endl;
        }
    }
};