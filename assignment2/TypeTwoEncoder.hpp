#include "output_stream.hpp"
#include "LZSSEncoder.hpp"
#include "prefix_codes.hpp"

// To compute CRC32 values, we can use this library
// from https://github.com/d-bahr/CRCpp
#define CRCPP_USE_CPP11
#include "CRC.h"

static const u32 b2_size {(1<<16)-1};

class TypeTwoEncoder{
    public:
    TypeTwoEncoder () {}

    void Encode(OutputBitStream& stream, u32 block_size, std::array <u8, b2_size>& block_contents, bool is_last){

        // Encode block w LZSS
        LZSSEncoder_2 lzss {};
        const std::vector<Code>& result = lzss.Encode(block_size, block_contents);
        // get symbol counts
        for(const Code& curr : result){
            if(curr.code_type == 1){
                incrementLiteralCount(curr.value);   
            }
            else if (curr.code_type == 2){
                incrementLengthCount(curr.value);
            }
            else if (curr.code_type == 3){
                incrementDistanceCount(curr.value); 
            }
            else{
                std::cerr << "invalid code type in result" << std::endl;
            }
            // count EOB 
            LLCount.at(256) = 1;
        }

        // get code
        //printLLCount();
        //printDistanceCount();
    }

    private:
    LLCodesBlock_1 ll_codes {};
    DistanceCodesBlock_1 distance_codes {};
    
    std::array <u32, 288> LLCount {};       // LLCount[symbol] = count of that symbol
    std::array <u32, 30> DistanceCount {};  // DistanceCount[symbol] = count of that

    void incrementLiteralCount(u32 literal){
        ++LLCount.at(literal);
    }
    void incrementLengthCount(u32 length){
        u32 symbol = ll_codes.getLengthSymbol(length);
        ++LLCount.at(symbol);
    }
    void incrementDistanceCount(u32 dist){
        u32 symbol = distance_codes.getDistanceSymbol(dist);
        ++DistanceCount.at(symbol);
    }
    void printLLCount(){
        for(u32 idx = 0; idx < 288; ++idx){
            std::cerr << "[" << idx << ", " << LLCount.at(idx) << "]"<< std::endl;
        }
    }
    void printDistanceCount(){
        for(u32 idx = 0; idx < 30; ++idx){
            std::cerr << "[" << idx << ", " << DistanceCount.at(idx) << "]"<< std::endl;
        }
    }
};