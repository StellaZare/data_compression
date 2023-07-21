#include <vector>
#include <array>
#include "output_stream.hpp"
#include "input_stream.hpp"
#include "discrete_cosine_transform.hpp"

namespace stream{

    void pushHeader(OutputBitStream& stream, dct::Quality q, u16 height, u16 width){
        stream.push_bits(q, 2);
        stream.push_u16(height);
        stream.push_u16(width);
    }

    void readHeader(InputBitStream& stream, dct::Quality& quality, u16& height, u16& width){
        u32 q = stream.read_bits(2);
        if(q == 0){
            quality = dct::Quality::low;
        }else if (q == 1){
            quality = dct::Quality::medium;
        }else{
            quality = dct::Quality::high;
        }

       height = stream.read_u16();
       width = stream.read_u16();
    }

    void pushBlock(OutputBitStream& stream, Array64 block){
        for(u32 idx = 0; idx < 64; idx++){
            if(block.at(idx) < 0){
                stream.push_bit(1);
                stream.push_u16(-1 * block.at(idx));
            }else{
                stream.push_bit(0);
                stream.push_u16(block.at(idx));
            }
        }
    }

    Array64 readBlock(InputBitStream& stream){
        Array64 block;
        for(u32 idx = 0; idx < 64; idx++){
            u16 sign = stream.read_bit();
            if(sign == 1){
                block.at(idx) = -1 * stream.read_u16();
            }else{
                block.at(idx) = stream.read_u16();
            }

        }

        return block;
    }
}