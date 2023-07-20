#include <vector>
#include <array>
#include "output_stream.hpp"
#include "discrete_cosine_transform.hpp"

namespace stream{

    void pushHeader(OutputBitStream& stream, dct::Quality q, u32 height, u32 width){
        stream.push_bits(q, 2);
        stream.push_u16(height);
        stream.push_u16(width);
    }

    void pushBlock(OutputBitStream& stream, Array64 block){
        for(u32 idx = 0; idx < 64; idx++){
            stream.push_u16(block.at(idx));
        }
    }
}