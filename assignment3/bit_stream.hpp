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

    void pushQuantizedArray(OutputBitStream& stream, Array64 array){
        for(u32 idx = 0; idx < 64; idx++){
            int num = array.at(idx);
            // sign bit
            bool sign = (num < 0) ? 1 : 0;
            stream.push_bit(sign);
            // value
            num = (num < 0) ? -1*num : num;
            stream.push_u16(num);
            // prints
            // std::cout << sign << num << " ";
            // if(idx != 0 && idx%8==0)
            //     std::cout<<std::endl;
        }
    }

    Array64 readQuantizedArray(InputBitStream& stream){
        Array64 block;
        for(u32 idx = 0; idx < 64; idx++){
            u16 sign = stream.read_bit();
            int num = stream.read_u16();
            block.at(idx) = (sign == 1) ? (-1*num) : num;

            // prints
            // std::cout << sign << num << " ";
            // if(idx != 0 && idx%8==0)
            //     std::cout << std::endl;
        }
        return block;
    }
}