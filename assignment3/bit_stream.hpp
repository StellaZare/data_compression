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
        }
    }

    void pushDelta(OutputBitStream& stream, u16 num){
        for(u16 i = 0; i < num; i++){
            std::cout << "1";
            stream.push_bit(1);
        }
        std::cout << "0\n";
        stream.push_bit(0);
    }

    void pushQuantizedArrayDelta(OutputBitStream& stream, Array64 array){
        for(u32 idx = 0; idx < 2; idx++){
            int num = array.at(idx);
            bool sign = (num < 0) ? 1 : 0;
            stream.push_bit(sign);
            // value
            num = (num < 0) ? -1*num : num;
            stream.push_u16(num);
            std::cout << "("<<sign<<")"<<num<<" ";
        }

        for(u32 idx = 2; idx < 64; idx++){
            int delta = array.at(idx) - array.at(idx-1);
            std::cout << delta << " ";
            if(delta == 0){
                stream.push_bit(0);
            }else if(delta > 0){
                stream.push_bits(2, 2);     // 10 for + delta 
                pushDelta(stream, delta);
            }else{
                stream.push_bits(3, 2);     // 11 for - delta
                pushDelta(stream, -1*delta);
            }  
        }
    }

    Array64 readQuantizedArray(InputBitStream& stream){
        Array64 block;
        for(u32 idx = 0; idx < 64; idx++){
            u16 sign = stream.read_bit();
            int num = stream.read_u16();
            block.at(idx) = (sign == 1) ? (-1*num) : num;
        }
        return block;
    }

    u16 readDelta(InputBitStream& stream){
        u16 value = 0;
        u16 bit = stream.read_bit();
        while(bit == 1){
            value++;
            bit = stream.read_bit();
        }
        return value;
    }

    Array64 readQuantizedArrayDelta(InputBitStream& stream){
        Array64 block;
        for(u32 idx = 0; idx < 2; idx++){
            u16 sign = stream.read_bit();
            int num = stream.read_u16();
            block.at(idx) = (sign == 1) ? (-1*num) : num;
        }

        for(u32 idx = 2; idx < 64; idx++){
            u16 bit1 = stream.read_bit();
            if(bit1 == 0){
                block.at(idx) = block.at(idx-1);
            }else{
                u16 bit2 = stream.read_bit();
                int num = readDelta(stream);
                if(bit2 == 0){
                    // positive delta
                    block.at(idx) = block.at(idx-1) + num;
                }else{
                    // negative delta 
                    block.at(idx) = block.at(idx-1) - num;
                }
            }

        }
        return block;
    }

    
}