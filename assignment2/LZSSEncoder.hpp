#ifndef LZSSENCODER_H
#define LZSSENCODER_H


#include <array>
#include <vector>
#include <cstdint>
#include <algorithm>
#include "prefix_codes.hpp"

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;

struct Code {
    u8 code_type {0};   // 1:literal 2:length 3:distance 0:default
    u32 value {0};      // 1: 0-255  2:3-258  3:1-32768  0:0
};


class LZSSEncoder_2 {
    public:

    LZSSEncoder_2() {}    // optionally talke lookahead dist and buffer size as input 

    static const u32 block_contents_size {(1<<16)-1};
    const std::vector<Code>& Encode(u32 block_size, std::array <u8, block_contents_size>& block_contents){
        // Fill lookahead
        if (bytes_processed < lookahead_distance){
            fillLookahead(block_contents, block_size, std::min(lookahead_distance, block_size));
        }

        // Begin encoding data
        while(curr_idx < block_size){

            std::array <u32, 2> back_ref = getBackRef();    // returns {length, distance}
            if(back_ref.at(0) == 0 && back_ref.at(1) == 0){
                // none found
                std::cerr << buffer.at(curr_idx%buffer_size) << std::endl;
                addLiteral(buffer.at(curr_idx%buffer_size));
                ++curr_idx;
                fillLookahead(block_contents, block_size);

            }else{
                // back ref found
                std::cerr << back_ref.at(0) << ":" << back_ref.at(1) << std::endl;
                addLengthDistancePair(back_ref);
                curr_idx += back_ref.at(0);
                fillLookahead(block_contents, block_size, back_ref.at(0));
            }
            if(bytes_processed >= buffer_size){
                history_idx = bytes_processed;
            }
        }

        return output_buffer;
    }

    const std::vector<Code>& getEncoded(){
        return output_buffer;
    }

    private:
    
    const u32 lookahead_distance {258};
    static const u32 buffer_size {5000};

    std::array <u8, buffer_size> buffer {};
    std::vector <Code> output_buffer {};
    u32 history_idx {0};
    u32 curr_idx    {0};
    u32 bytes_processed {0};

    /*
        Adds lookahead data to buffer from block contents
        numElement - how many indeces to fill
    */
    void fillLookahead(std::array <u8, block_contents_size >& block_contents,const u32& block_size, u32 num_elements = 1){
        for(u32 num = 0; num < num_elements && bytes_processed < block_size; ++num){
            buffer.at(bytes_processed %buffer_size) = block_contents.at(bytes_processed);
            ++bytes_processed;
        }
    }

    /*
        Finds max back reference for string starting at curr_idx
        Returns an array {length value, distance value}
    */
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

    /*
        Finds the length of the match for strings starting at curr_idx and check_id
        Returns the length value
    */
    u32 getMatchLength(u32 check_idx){
        u32 length {0};
        while(buffer.at((check_idx+length) %buffer_size) == buffer.at((curr_idx+length) %buffer_size) && curr_idx + length < bytes_processed){
            ++length;
        }
        return length;
    }

    /*
        Adds literal value to output_buffer
    */
    void addLiteral(u8 literal){
        output_buffer.push_back(Code{1, literal});
    }

    /*
        Adds length distance pair values to output_buffer
    */
    void addLengthDistancePair(std::array <u32, 2> back_ref){
        // add length
        output_buffer.push_back(Code{2, back_ref.at(0)});
        // add distance
        output_buffer.push_back(Code{3, back_ref.at(1)});
    }

};

#endif