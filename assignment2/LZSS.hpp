#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include "output_stream.hpp"

#define CRCPP_USE_CPP11
#include "CRC.h"

class LZSSEncoder{
    public:

    LZSSEncoder(OutputBitStream& stream){}

    static const u32 buffer_size = (1<<16)-1;

    void encodeBlock(OutputBitStream& stream, std::array <u8, buffer_size >& block_contents, const u32& block_size){
        u32 bytes_processed = 0;

        // first block -> fill look ahead
        if (add_lookahead < lookahead_distance){
            for(bytes_processed = 0; bytes_processed < lookahead_distance && bytes_processed < block_size; ++bytes_processed){
                buff.at(add_lookahead++) = block_contents.at(bytes_processed);
            }
        }

        while(bytes_processed < block_size){
            // find backreference
            if(curr_symbol == 0){
                stream.
            }
            for(uint idx = curr_symbol-1 ; )

            break;
        }
    }  

    private:
    const u32 lookahead_distance {256};

    std::array <u8, 500> buff {};
    std::array <u8, 256> last_seen{};
    std::size_t start_history {0};
    std::size_t add_lookahead {0};
    std::size_t curr_symbol {0};
};