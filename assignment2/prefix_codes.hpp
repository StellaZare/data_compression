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
#include <cassert>
#include <algorithm>

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

class DynamicCodes {
    public:
    DynamicCodes(const std::vector<u32>& input_length_table) : length_table{input_length_table} {
        symbol_encoding = construct_canonical_code(length_table);
    }

    std::vector<bool> getCodeSequence(u32 symbol) const{
        std::vector <bool> code_sequence {};
        u32 num_bits {length_table.at(symbol)};
        u32 code_bits {symbol_encoding.at(symbol)};
        for(u32 idx = 0; idx < num_bits; ++idx){
            code_sequence.push_back(code_bits >> (num_bits - idx - 1) & 1);
        }
        std::cerr << "symbol: " << symbol << " " << num_bits << " " << code_bits << std::endl;
        return code_sequence;
    }

    private:
    std::vector <u32> length_table {};
    std::vector <u32> symbol_encoding {};

    //Given a vector of lengths where lengths.at(i) is the code length for symbol i,
    //returns a vector V of unsigned int values, such that the lower lengths.at(i) bits of V.at(i)
    //comprise the bit encoding for symbol i (using the encoding construction given in RFC 1951). Note that the encoding is in 
    //MSB -> LSB order (that is, the first bit of the prefix code is bit number lengths.at(i) - 1 and the last bit is bit number 0).
    //The codes for symbols with length zero are undefined.
    std::vector< u32 > construct_canonical_code( std::vector<u32> const & lengths ){

        unsigned int size = lengths.size();
        std::vector< unsigned int > length_counts(16,0); //Lengths must be less than 16 for DEFLATE
        u32 max_length = 0;
        for(auto i: lengths){
            assert(i <= 15);
            length_counts.at(i)++;
            max_length = std::max(i, max_length);
        }
        length_counts[0] = 0; //Disregard any codes with alleged zero length

        std::vector< u32 > result_codes(size,0);

        //The algorithm below follows the pseudocode in RFC 1951
        std::vector< unsigned int > next_code(size,0);
        {
            //Step 1: Determine the first code for each length
            unsigned int code = 0;
            for(unsigned int i = 1; i <= max_length; i++){
                code = (code+length_counts.at(i-1))<<1;
                next_code.at(i) = code;
            }
        }
        {
            //Step 2: Assign the code for each symbol, with codes of the same length being
            //        consecutive and ordered lexicographically by the symbol to which they are assigned.
            for(unsigned int symbol = 0; symbol < size; symbol++){
                unsigned int length = lengths.at(symbol);
                if (length > 0)
                    result_codes.at(symbol) = next_code.at(length)++;
            }  
        } 
        return result_codes;
    }

};

class PackageMerge {
    public:

    PackageMerge () {};
    
    std::vector<u32> getSymbolLengths(std::vector < std::pair <std::vector<u32>, double> > probabilities) {
        u32 m = probabilities.size();
        // printProbabilities(probabilities);
        // std::cerr << " ------- " << std::endl;

        // while there are not sufficient packages
        while(probabilities.size() < (2*m)-2){
            // sort with increasing probabilities
            sort(probabilities.begin(), probabilities.end(), sortByProb);
            // if odd number of packages -> discard last
            if(probabilities.size()%2 != 0 && probabilities.at(probabilities.size()-1).first.size() > 1){
                probabilities.pop_back();
            }
            u32 current_size = probabilities.size();
            for(u32 idx = 0; idx <  current_size; idx += 2){
                // create package
                std::vector<u32> v {};
                appendVectors(v, probabilities.at(idx).first, probabilities.at(idx+1).first);
                double prob = probabilities.at(idx).second + probabilities.at(idx+1).second;
                // merge
                probabilities.push_back({v, prob});
            }

            // printProbabilities(probabilities);
            // std::cerr << " ------- " << std::endl;
        }
        std::vector<u32> lengths_table {};
        for(u32 symbol = 0; symbol < 288; ++symbol){
            lengths_table.push_back(countOccurances(probabilities, symbol));
        }

        // printLengths(lengths_table);

        return lengths_table;
    }

    private:

    static bool sortByProb(const std::pair <std::vector<u32>, double> &a, const std::pair <std::vector<u32>, double> &b){
        return (a.second < b.second);
    }

    void printProbabilities(std::vector < std::pair <std::vector<u32>, double> >& probabilities){
        for (const auto& pair : probabilities) {
            std::cout << "Literal: ";
            for (const auto& symbol : pair.first) {
                std::cout << symbol << " ";
            }
            std::cout << "prob: " << pair.second << std::endl;
        }
    }

    void appendVectors(std::vector<u32>& dest, const std::vector<u32>& src1, const std::vector<u32>& src2){
        for(const u32 item : src1){
            dest.push_back(item);
        }
        for(const u32 item : src2){
            dest.push_back(item);
        }
    }

    u32 countOccurances(std::vector < std::pair <std::vector<u32>, double> >& probabilities, u32 symbol){
        u32 count {0};
        for (const auto& pair : probabilities){         // traverse properties vector
            for (const auto& curr_symbol : pair.first) {     // traverse set of symbol
                if (curr_symbol == symbol){
                    ++count;
                }
            }
        }
        return count;
    }

    void printLengths(const std::vector<u32>& lengths_table){
        for(u32 symbol = 0; symbol < lengths_table.size(); ++symbol){
            std::cerr << symbol << "-" << lengths_table.at(symbol) << std::endl;
        }
    }
};





#endif