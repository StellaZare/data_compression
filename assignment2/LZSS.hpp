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

    LZSSEncoder(OutputBitStream& input_stream) : stream{input_stream} {
        stream.push_bit(1); 
        stream.push_bits(1, 2); //Two bit block type 
    }

    static const u32 block_contents_size = (1<<16)-1;

    void encodeBlock(std::array <u8, block_contents_size >& block_contents, const u32& block_size){
        u32 bytes_processed = 0;

        // first block -> fill look ahead
        if (lookahead_idx < lookahead_distance){
            for(bytes_processed = 0; bytes_processed < lookahead_distance && bytes_processed < block_size; ++bytes_processed){
                buff.at(lookahead_idx++) = block_contents.at(bytes_processed);
            }
        }

        for(int idx = 0; idx < lookahead_idx; ++idx){
            std::cerr << buff.at(idx) << " ";
        }

        bytes_processed = 0;
        while(bytes_processed < block_size){
            // push first 3 literals
            while(curr_idx < 3){
                std::cerr << block_contents.at(bytes_processed) <<"\n";
                pushLiteral(block_contents.at(bytes_processed));
                ++bytes_processed;
                ++curr_idx;
            }

            // look for back reference [length, distance]
            std::array <u32, 2> back_ref = getBackRef();
            if(back_ref.at(0) == 0 && back_ref.at(1) == 0){
                // none found
                std::cerr << block_contents.at(bytes_processed) <<"\n";
                pushLiteral(block_contents.at(bytes_processed++));
                ++curr_idx;
            }else{
                // back ref found
                std::cerr << back_ref.at(0) << ":" << back_ref.at(1) <<"\n";
                pushLengthDistance(back_ref);
                curr_idx += back_ref.at(0);
                bytes_processed += back_ref.at(0);
            }
        }
    }  

    void pushEOB(){
        std::vector<bool> EOB = ll_codes.getCodeSequence(256);
        for(auto bit : EOB){
            stream.push_bit(bit);
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
    OutputBitStream& stream;

    void pushLengthDistance(const std::array <u32, 2>& back_ref){
        u32 length_symbol = ll_codes.getLengthSymbol(back_ref.at(0));
        u32 base_length = ll_codes.getBaseLength(back_ref.at(0));
        u32 length_offset = ll_codes.getLengthOffset(length_symbol);

        for(u8 bit : ll_codes.getCodeSequence(length_symbol)){
            stream.push_bit(bit);
        }

        stream.push_bits(back_ref.at(0) - base_length, length_offset);

        u32 distance_symbol = distance_codes.getDistanceSymbol(back_ref.at(1));
        u32 base_distance = distance_codes.getBaseDistance(back_ref.at(1));
        u32 distance_offset = distance_codes.getNumOffset(distance_symbol);

        for(u8 bit : distance_codes.getCodeSequence(distance_symbol)){
            stream.push_bit(bit);
        }

        stream.push_bits(back_ref.at(0) - base_distance, distance_offset);
    }

    void pushLiteral(u8 literal){
        std::vector<bool> seq = ll_codes.getCodeSequence(literal);
        for(u32 bit : seq){
            stream.push_bit(bit);
        }
    }

    std::array <u32, 2> getBackRef(){
        u32 max_match_length {0};
        u32 max_match_distance {0};
        u32 match_length = {0};
        for(int check_idx = curr_idx-1; check_idx >= int(history_idx); --check_idx){
            
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