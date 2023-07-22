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

    void pushValue(OutputBitStream& stream, int num){
        std::cout << "[" << num << "]" << std::endl;
        if(num < 0){
            // negative value push (1) value
            stream.push_bit(1);
            stream.push_u16(-1*num);
        }else{
            // positive or zero value push (0) value
            stream.push_bit(0);
            stream.push_u16(num);
        }
    }

    void pushQuantizedArray(OutputBitStream& stream, Array64 array){
        for(u32 idx = 0; idx < 64; idx++){
            pushValue(stream, array.at(idx));
        }
    }

    void pushDeltaValue(OutputBitStream& stream, int num){
        std::cout << "[" << num << "]";
        if(num > 0){
            // positive start with 10
            std::cout << "10";
            stream.push_bits(2, 2);
        }else if(num < 0){
            //negative start with 11
            std::cout << "11";
            stream.push_bits(3, 2);
            num *= -1;
        }

        for(u16 i = 0; i < num-1; i++){
            std::cout << "1";
            stream.push_bit(1);
        }
        std::cout << "0" << " ";
        stream.push_bit(0);
    }

    Array64 quantizedToDelta(const Array64& quantized){
        Array64 delta_values;
        delta_values.at(0) = quantized.at(0);
        delta_values.at(1) = quantized.at(1);

        for(u32 idx = 2; idx < 64; idx++){
            delta_values.at(idx) = quantized.at(idx) - quantized.at(idx-1);
        }
        return delta_values;
    }

    void pushQuantizedArrayDelta(OutputBitStream& stream, Array64 array){
        Array64 delta_values = quantizedToDelta(array);
        dct::print_array(delta_values);

        // push first 2 as normal
        pushValue(stream, delta_values.at(0));
        pushValue(stream, delta_values.at(1));
        
        for(u32 idx = 2; idx < 64; idx++){
            pushDeltaValue(stream, delta_values.at(idx));
        }
    }

    /* ----- Decompressor code -----*/

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

    int readValue(InputBitStream& stream){
        // (+) sign = 0    (-) sign = 1
        bool sign = stream.read_bit();
        int num  = stream.read_u16();
        num = (sign == 1) ? (-1*num) : num ;
        std::cout << num << " ";
        return num;
    }

    Array64 readQuantizedArray(InputBitStream& stream){
        Array64 block;
        for(u32 idx = 0; idx < 64; idx++){
            block.at(idx) = readValue(stream);
        }
        return block;
    }

    int readDeltaValue(InputBitStream& stream){
        if(stream.read_bit() == 0){
            return 0;
        }
        // 0 --> (+)   and 1 --> (-)
        bool sign = stream.read_bit();
        int num = 1;
        bool bit = stream.read_bit();
        while(bit == 1){
            num++;
            bit = stream.read_bit();
        }
        num = (sign == 0) ? (num) : -1*(num);
        std::cout << num << " ";
        return num;
    }

    Array64 deltaToQuantized(const Array64& delta){
        Array64 quantized;
        quantized.at(0) = delta.at(0);
        quantized.at(1) = delta.at(1);

        for(u32 idx = 2; idx < 64; idx++){
            quantized.at(idx) = quantized.at(idx-1) + delta.at(idx);
        }
        return quantized;
    }

    Array64 readQuantizedArrayDelta(InputBitStream& stream){
        Array64 delta_values;
        delta_values.at(0) = readValue(stream); std::cout <<std::endl;
        delta_values.at(1) = readValue(stream); std::cout <<std::endl;

        for(u32 idx = 2; idx < 64; idx++){
            delta_values.at(idx) = readDeltaValue(stream);
        }
        Array64 quantized = deltaToQuantized(delta_values);
        std::cout <<std::endl;
        dct::print_array(delta_values);
        return quantized;
    }

    
}