#include <vector>
#include <array>
#include "stream.hpp"

namespace stream{

    void push_header(OutputBitStream& stream, dct::Quality q, u16 height, u16 width){
        stream.push_bits(q, 2);
        stream.push_u16(height);
        stream.push_u16(width);
    }

    void push_value(OutputBitStream& stream, int num){
        if(num < 0){
            // negative value push (1) value
            stream.push_bit(1);
            stream.push_bit(1);
            stream.push_u16(-1*num);
        }else{
            // positive or zero value push (0) value
            stream.push_bit(1);
            stream.push_bit(0);
            stream.push_u16(num);
        }
    }

    void push_value_n(OutputBitStream& stream, int value, u16 num_bits){
        if(value < 0){
            // negative value push (1) value
            stream.push_bit(1);
            stream.push_bit(1);
            stream.push_bits((-1*value), num_bits);
        }else{
            // positive or zero value push (0) value
            stream.push_bit(1);
            stream.push_bit(0);
            stream.push_bits((value), num_bits);
        }
    }

    void push_delta_value(OutputBitStream& stream, int num){
        if(num > 0){
            // positive start with 10
            stream.push_bit(1);
            stream.push_bit(0);
        }else if(num < 0){
            //negative start with 11
            stream.push_bit(1);
            stream.push_bit(1);
            num *= -1;
        }
        for(u16 i = 0; i < num-1; i++){
            stream.push_bit(1);
        }
        stream.push_bit(0);
    }

    void push_quantized_array(OutputBitStream& stream, const Array64& array){
        for(u32 idx = 0; idx < 64; idx++){
            push_value(stream, array.at(idx));
        }
    }

    u32 push_RLE_zeros(OutputBitStream& stream, const Array64& array, u32 start){
        u32 idx = start; 
        u32 count = 0;
        while(idx < 64 && array.at(idx) == 0){
            count++;
            idx++;
        }

        // push initial 0 then count
        stream.push_bit(0);
        stream.push_bits(count, 6);
        return count;
    }

    void push_motion_vector_RLE(OutputBitStream& stream, const std::vector<int>& mv){
        // push first vector normally
        push_value_n(stream, mv.at(0), 4);
        push_value_n(stream, mv.at(1), 4);

        u32 idx = 2;
        while(idx < mv.size()){
            if(mv.at(idx) != 0){
                push_delta_value(stream, mv.at(idx++));
            }else{
                u32 count = idx +1;
                while(count < mv.size() && mv.at(count) == 0 && count-idx < 8){
                    count++;
                }
                u32 num_zeros = count - idx -1;
                // push initial 0 then count
                stream.push_bit(0);
                stream.push_bits(num_zeros, 3);
                idx = count;
            }
        }
    }

    Array64 quantized_to_delta(const Array64& quantized){
        Array64 delta_values;
        delta_values.at(0) = quantized.at(0);
        delta_values.at(1) = quantized.at(1);

        for(u32 idx = 2; idx < 64; idx++){
            delta_values.at(idx) = quantized.at(idx) - quantized.at(idx-1);
        }
        return delta_values;
    }

    void push_quantized_array_delta(OutputBitStream& stream, const Array64& array){
        Array64 delta_values = quantized_to_delta(array);

        // push first 2 as normal
        push_value(stream, delta_values.at(0));
        push_value(stream, delta_values.at(1));

        u32 idx = 2;
        while(idx < 64){
            if(delta_values.at(idx) == 0){
                idx += push_RLE_zeros(stream, delta_values, idx+1);
            }else{
                push_delta_value(stream, delta_values.at(idx));
            }           
            idx++;
        }
    }

    /* ----- Decompressor code -----*/

    void read_header(InputBitStream& stream, dct::Quality& quality, u16& height, u16& width){
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

    int read_value(InputBitStream& stream){
        // (+) sign = 0    (-) sign = 1
        bool sign = stream.read_bit();
        sign = stream.read_bit();
        int num  = stream.read_u16();
        num = (sign == 1) ? (-1*num) : num ;
        return num;
    }

    int read_value_n(InputBitStream& stream, u16 num_bits){
        // (+) sign = 0    (-) sign = 1
        bool sign = stream.read_bit();
        sign = stream.read_bit();
        int num  = stream.read_bits(num_bits);
        num = (sign == 1) ? (-1*num) : num ;
        return num;
    }

    int read_delta_value(InputBitStream& stream){
        if(stream.read_bit() == 0){
            return 0;
        }
        // 0 --> (+)   and 1 --> (-)
        bool sign = stream.read_bit();
        int num = 1;
        while(stream.read_bit()){
            num++;
        }
        num = (sign == 1) ? -1*(num) : num;
        return num;
    }

    Array64 read_quantized_array(InputBitStream& stream){
        Array64 block;
        for(u32 idx = 0; idx < 64; idx++){
            block.at(idx) = read_value(stream);
        }
        return block;
    }

    Array64 delta_to_quantized(const Array64& delta){
        Array64 quantized;
        quantized.at(0) = delta.at(0);
        quantized.at(1) = delta.at(1);

        for(u32 idx = 2; idx < 64; idx++){
            quantized.at(idx) = quantized.at(idx-1) + delta.at(idx);
        }
        return quantized;
    }

    void add_RLE_zeros(Array64& delta_values, u32 start, u32 count){
        for(u32 idx = start; idx < 64 && idx < start+count; idx++){
            delta_values.at(idx) = 0;
        }
    }

    Array64 read_quantized_array_delta(InputBitStream& stream){
        Array64 delta_values;
        delta_values.at(0) = read_value(stream); 
        delta_values.at(1) = read_value(stream); 

        u32 idx = 2;
        while(idx < 64){
            int delta = read_delta_value(stream);
            delta_values.at(idx) = delta;
            idx++;
            if(delta == 0){
                u32 count = stream.read_bits(6);
                add_RLE_zeros(delta_values, idx, count);
                idx+=count;
            }
        }

        Array64 quantized = delta_to_quantized(delta_values);
        return quantized;
    }

    std::vector<int> read_motion_vector_RLE(InputBitStream& stream, int num_vectors){
        std::vector<int> mv;

        // push first vector normally
        mv.push_back(read_value_n(stream, 4));
        mv.push_back(read_value_n(stream, 4));

        u32 idx = 2;
        while(idx < (num_vectors * 2)){
            int value = read_delta_value(stream);
            mv.push_back(value);
            if(value == 0){
                u32 num_zeros = stream.read_bits(3);
                for(u32 count = 0; count < num_zeros; count++){
                    mv.push_back(0);
                }
                idx += num_zeros;
            }
            idx++;
        }
        return mv;
    }

}