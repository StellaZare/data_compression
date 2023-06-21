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

    LZSSEncoder(OutputBitStream& input_stream, bool last_block) : stream{input_stream} {
        stream.push_bit(last_block); 
        stream.push_bits(1, 2); //Two bit block type 
    }

    static const u32 block_contents_size {(1<<16)-1};
    void encodeBlock(std::array <u8, block_contents_size >& block_contents, const u32& block_size){
        // first block -> fill look ahead
        if (bytes_processed < lookahead_distance){
            fillLookahead(block_contents, block_size, std::min(lookahead_distance, block_size));
        }

        while(curr_idx < block_size){
            // look for back reference [length, distance]
            std::array <u32, 2> back_ref = getBackRef();
            if(back_ref.at(0) == 0 && back_ref.at(1) == 0){
                // none found
                pushLiteral(buffer.at(curr_idx%buffer_size));
                ++curr_idx;
                fillLookahead(block_contents, block_size);

            }else{
                // back ref found
                pushLengthDistance(back_ref);
                curr_idx += back_ref.at(0);
                fillLookahead(block_contents, block_size, back_ref.at(0));
            }
            if(bytes_processed >= buffer_size){
                history_idx = bytes_processed;
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
    const u32 lookahead_distance {258};
    static const u32 buffer_size {5000};

    std::array <u8, buffer_size> buffer {};
    // std::array <u8, 256> last_seen{};
    std::size_t history_idx {0};
    std::size_t curr_idx {0};
    std::size_t bytes_processed {0};

    LLCodesBlock_1 ll_codes {};
    DistanceCodesBlock_1 distance_codes {};
    OutputBitStream& stream;

    void fillLookahead(std::array <u8, block_contents_size >& block_contents,const u32& block_size, u32 num_elements = 1){
        for(u32 num = 0; num < num_elements && bytes_processed < block_size; ++num){
            buffer.at(bytes_processed %buffer_size) = block_contents.at(bytes_processed);
            ++bytes_processed;
        }
    }

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

        stream.push_bits(back_ref.at(1) - base_distance, distance_offset);
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
        for(int check_idx = (curr_idx-1); check_idx >= int(history_idx); --check_idx){
            if(buffer.at(check_idx %buffer_size) == buffer.at(curr_idx %buffer_size) &&
               buffer.at((check_idx+1) %buffer_size) == buffer.at((curr_idx+1) %buffer_size) &&
               buffer.at((check_idx+2) %buffer_size) == buffer.at((curr_idx+2) %buffer_size)){

                match_length = getMatchLength(check_idx);
                if(match_length > max_match_length){
                    max_match_length = match_length;
                    max_match_distance = (curr_idx) - check_idx;
                } 
            }
        }
        return {max_match_length, max_match_distance};
        
    }

    u32 getMatchLength(std::size_t check_idx){
        u32 length {0};
        while(buffer.at((check_idx+length) %buffer_size) == buffer.at((curr_idx+length) %buffer_size) && curr_idx + length < bytes_processed){
            ++length;
        }
        return length;
    }
};