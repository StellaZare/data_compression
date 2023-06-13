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
