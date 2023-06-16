#include "output_stream.hpp"
#include <iostream>
#include <unordered_map>
#include <string>
#include <bitset>
#include <vector>
#include <cmath>
#include <stdexcept>

class LLCodesBlock_1 {

    public:

    LLCodesBlock_1(){
        u32 symbol = 0;
        for(symbol = 0; symbol < 144; ++symbol){
            create_sequence(symbol, 8, 0b00110000 + symbol);   
        }
        for(symbol = 144; symbol < 256; ++symbol){
            create_sequence(symbol, 9, 0b110010000 + (symbol - 144));   
        }
        for(symbol = 256; symbol < 280; ++symbol){
            create_sequence(symbol, 7, (symbol - 256));   
        }
        for(symbol = 280; symbol < 288; ++symbol){
            create_sequence(symbol, 8, 0b11000000 + (symbol - 280));   
        }
    }

    std::vector<bool> getCodeSequence(u32 symbol) const {
        return code_sequence.at(symbol);
    }

    private:
    std::array < std::vector<bool>, 288> code_sequence {};

    void create_sequence(u32 symbol, u32 num_bits, u32 code_bits){
        for(u32 idx = 0; idx < num_bits; ++idx){
            code_sequence.at(symbol).push_back(code_bits >> (num_bits - idx - 1) & 1);
        }
    }

};

class DistanceCodesBlock_1 {

    public:

    DistanceCodesBlock_1(){
        u32 symbol = 0;
        for(symbol = 0; symbol < 30; ++symbol){
            create_sequence(symbol, 5, symbol);   
        }  
    }

    /*
        param: distance symbol
        return: vector of 5 bits to represent the symbol
    */
    std::vector<bool> getCodeSequence(u32 symbol) const {
        return code_sequence.at(symbol);
    }

    /*
        param: distance symbol
        return: number of offsets bits for the symbol
    */
    u32 getNumOffset(u8 symbol) const {
        return offset_bits.at(symbol);
    }

    /*
        param: distance of back reference
        return: symbol corresponding to the distance
    */
    u32 getDistanceSymbol (u32 distance){
        u32 idx = 0;
        while(base_distance.at(idx) <= distance){
            ++idx;
        }
        return idx-1;
    }

    private:
    std::array < std::vector<bool>, 30> code_sequence {};
    
    std::array < u32, 30> base_distance {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537,
        2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
    };
    std::array < u32, 30 > offset_bits {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };


    /*
        param: idx of location in code_sequence
        param: number of bits used in code_sequence
        param: bit representation of value
    */
    void create_sequence(u32 symbol, u32 num_bits, u32 code_bits){
        for(u32 idx = 0; idx < num_bits; ++idx){
            code_sequence.at(symbol).push_back(code_bits >> (num_bits - idx - 1) & 1);
        }
    }

};
