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
                std::cerr << "invalid code type in result" << std::endl;
            }
            // count EOB 
            LL_count.at(256) = 1;
        }
        // get symbol probabilities 
        getLLProbabilities();
        getDistanceProbabilities();
        std::cerr << "LL " << LL_probability.size();

        //printProbabilities();

        // get symbol lengths
        PackageMerge pm {};
        DynamicCodes LL_codes {pm.getSymbolLengths(LL_probability)};
        DynamicCodes distance_codes {pm.getSymbolLengths(distance_probability)};
        
        
        std::vector<bool> c = LL_codes.getCodeSequence(212);
        for(bool bit:c){
            std::cerr << bit;
        }

    }

    private:
    LLCodesBlock_1 ll_codes {};
    DistanceCodesBlock_1 distance_codes {};
    
    std::array <u32, 288> LL_count {};       // LL_count[symbol] = count of that symbol
    std::array <u32, 30> distance_count {};  // distance_count[symbol] = count of that
    double LL_total_count {};
    double distance_total_count {};

    std::vector < std::pair <std::vector<u32>, double> > LL_probability {};
    std::vector < std::pair <std::vector<u32>, double> > distance_probability {};

    void incrementLiteralCount(u32 literal){
        ++LL_count.at(literal);
    }
    void incrementLengthCount(u32 length){
        u32 symbol = ll_codes.getLengthSymbol(length);
        ++LL_count.at(symbol);
    }
    void incrementDistanceCount(u32 dist){
        u32 symbol = distance_codes.getDistanceSymbol(dist);
        ++distance_count.at(symbol);
    }
    void printLLCount(){
        for(u32 idx = 0; idx < 288; ++idx){
            std::cerr << "[" << idx << ", " << LL_count.at(idx) << "]"<< std::endl;
        }
    }
    void printDistanceCount(){
        for(u32 idx = 0; idx < 30; ++idx){
            std::cerr << "[" << idx << ", " << distance_count.at(idx) << "]"<< std::endl;
        }
    }

    void getLLProbabilities(){
        for(u32 idx = 0; idx < 288; ++idx){
            if(LL_count.at(idx) != 0){
                double prob = LL_count.at(idx) / LL_total_count;
                LL_probability.push_back({{idx}, prob});
            }
        }
    }
    void getDistanceProbabilities(){
        for(u32 idx = 0; idx < 30; ++idx){
            if(distance_count.at(idx) != 0){
                double prob = distance_count.at(idx) / distance_total_count;
                distance_probability.push_back({{idx}, prob});
            }
        }
    }
    // void printProbabilities(){
    //     for (const auto& pair : LL_probability) {
    //         std::cout << "Literal: ";
    //         for (const auto& symbol : pair.first) {
    //             std::cout << symbol << " ";
    //         }
    //         std::cout << "prob: " << pair.second << std::endl;
    //     }

    //     for (const auto& pair : distance_probability) {
    //         std::cout << "Distance: ";
    //         for (const auto& symbol : pair.first) {
    //             std::cout << symbol << " ";
    //         }
    //         std::cout << "prob: " << pair.second << std::endl;
    //     }
    // }
};