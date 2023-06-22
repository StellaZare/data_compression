#ifndef PREFIX_CODES_H
#define PREFIX_CODES_H

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

    /*
        param: length symbol
        return: number of offsets bits for the symbol
    */
    u32 getLengthOffset(u32 symbol) const {
        return base_length.at(symbol-257).at(2);
    }

    /*
        param: length of back reference
        return: symbol corresponding to the distance
    */
    u32 getLengthSymbol (u32 length) const {
        u32 idx = 0;
        if(length == 258){
            return 285;
        }
        while(base_length.at(idx).at(1) <= length){
            ++idx;
        }
        return 257+idx-1;
    }

    /*
        param: base length of lengt
        return: symbol corresponding to the distance
    */
    u32 getBaseLength (u32 length) const {
        u32 idx = 0;
        if(length == 258){
            return 258;
        }
        while(base_length.at(idx).at(1) <= length){
            ++idx;
        }
        return base_length.at(idx-1).at(1);
    }

    private:
    std::array < std::vector<bool>, 288> code_sequence {};

    std::array < std::array< u32, 3> , 30> base_length {{
        {257, 3, 0},    {258, 4, 0},    {259, 5, 0},
        {260, 6, 0},    {261, 7, 0},    {262, 8, 0},
        {263, 9, 0},    {264, 10, 0},   {265, 11, 1},
        {266, 13, 1},   {267, 15, 1},   {268, 17, 1},
        {269, 19, 2},   {270, 23, 2},   {271, 27, 2},
        {272, 31, 2},   {273, 35, 3},   {274, 43, 3},
        {275, 51, 3},   {276, 59, 3},   {277, 67, 4},
        {278, 83, 4},   {279, 99, 4},   {280, 115, 4},
        {281, 131, 5},  {282, 163, 5},  {283, 195, 5},
        {284, 227, 5},  {285, 258, 0}
    }};

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
        try{
            return code_sequence.at(symbol);
        }catch(...){
            std::cerr << "getCodeSequence" << symbol << "\n";
            exit(1);
        }
    }

    /*
        param: distance symbol
        return: number of offsets bits for the symbol
    */
    u32 getNumOffset(u32 symbol) const {
        try{
            return offset_bits.at(symbol);
        }catch(...){
            std::cerr << "getNumOffset" << symbol << "\n";
            exit(1);
        }
    }

    /*
        param: distance of back reference
        return: symbol corresponding to the distance
    */
    u32 getDistanceSymbol (u32 distance) const {
        u32 idx = 0;
        while(base_distance.at(idx) <= distance){
            ++idx;
        }
        return idx-1;
    }

    /*
        param: distance of back reference
        return: base distance 
    */
    u32 getBaseDistance (u32 distance) const {
        u32 idx = 0;
        while(base_distance.at(idx) <= distance){
            ++idx;
        }
        return base_distance.at(idx-1);
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


#endif