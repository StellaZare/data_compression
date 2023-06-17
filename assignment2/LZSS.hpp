#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include "output_stream.hpp"
#include "prefix_codes.hpp"

#define CRCPP_USE_CPP11
#include "CRC.h"

class LZSSEncoder{
    public:

    LZSSEncoder(OutputBitStream& stream){}

    static const u32 block_contents_size = (1<<16)-1;

    void encodeBlock(OutputBitStream& stream, std::array <u8, block_contents_size >& block_contents, const u32& block_size){
        u32 bytes_processed = 0;

        // first block -> fill look ahead
        if (lookahead_idx < lookahead_distance){
            for(bytes_processed = 0; bytes_processed < lookahead_distance && bytes_processed < block_size; ++bytes_processed){
                buff.at(lookahead_idx++) = block_contents.at(bytes_processed);
            }
        }

        while(bytes_processed < block_size){
            // push first 3 literals
            while(curr_idx < 3){
                pushLiteral(stream, block_contents.at(bytes_processed++));
                ++curr_idx;
            }

            // look for back reference
            std::array <u32, 2> back_ref = getBackRef();
            if(back_ref.at(0) == 0 && back_ref.at(1) == 0){
                // none found
                pushLiteral(stream, block_contents.at(bytes_processed++));
                ++curr_idx;
            }else{
                // back ref found
                pushLengthDistance(stream, back_ref);
            }

            break;
        }
    }  

    private:
    const u32 lookahead_distance {256};
    static const u32 buff_size {(1<<16)-1};

    std::array <u8, buff_size> buff {};
    // std::array <u8, 256> last_seen{};
    std::size_t history_idx {0};
    std::size_t lookahead_idx {0};
    std::size_t curr_idx {0};

    LLCodesBlock_1 ll_codes {};
    DistanceCodesBlock_1 distance_codes {};

    void pushLengthDistance(OutputBitStream& stream, const std::array <u32, 2>& back_ref){
        u32 length_symbol = ll_codes.getLengthSymbol(back_ref.at(0));
        
    }

    void pushLiteral(OutputBitStream& stream, u8 literal){
        std::vector<bool> seq = ll_codes.getCodeSequence(literal);
        for(u32 bit : seq){
            stream.push_bit(bit);
        }
    }

    std::array <u32, 2> getBackRef(){
        u32 max_match_length {0};
        u32 max_match_distance {0};
        u32 match_length = {0};

        for(std::size_t check_idx = curr_idx-1; check_idx <= history_idx; --check_idx){
            if(buff.at(check_idx %buff_size) == buff.at(curr_idx %buff_size) &&
               buff.at(check_idx+1 %buff_size) == buff.at(curr_idx+1 %buff_size) &&
               buff.at(check_idx+2 %buff_size) == buff.at(curr_idx+2 %buff_size)){

                match_length = getMatchLength(check_idx);
                if(match_length > max_match_length){
                    max_match_length = match_length;
                    max_match_distance = curr_idx - check_idx;
                } 
            }
            if(max_match_length > 5){
                break;
            }
        }
        return {max_match_length, max_match_distance};
        
    }

    u32 getMatchLength(std::size_t check_idx){
        u32 length {0};
        while(buff.at(check_idx+length %buff_size) == buff.at(curr_idx+length %buff_size)){
            ++length;
        }
        return length;
    }
};