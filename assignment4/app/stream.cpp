#include <vector>
#include <array>
#include "stream.hpp"

namespace stream{

    std::map<int,int> delta_frequency {};
    std::map<int,int> RLE_frequency {}; 
    std::map<int, int> huffman_frequency {};

    std::map<int, u32> symbol_length {
        {-100, 8},   // negative escape symbol
        {-5, 9},  
        {-4, 9},  
        {-3, 7},  
        {-2, 5},  
        {-1, 2},  
        {0, 2},   
        {1, 2},   
        {2, 5},   
        {3, 7},   
        {4, 9},   
        {5, 9},   
        {100, 8},    // positive escape symbol
        {110, 6},    // 4 zeros
        {120, 6},    // 8 zeros
        {150, 3}     // EOB - the rest of the block is zeros
    };

    std::map<int, u32> symbol_encoding {
        {-100, 218},   
        {-5, 394},     
        {-4, 438},     
        {-3, 108},     
        {-2, 26},      
        {-1, 1},       
        {0, 0},     
        {1, 2},     
        {2, 25},    
        {3, 99},    
        {4, 439},   
        {5, 395},   
        {100, 196},   
        {110, 30},    
        {120, 55},    
        {150, 7}      
    };

    std::map<int, u32> encoding_symbol {
        {218, -100},
        {394, -5},
        {438, -4},
        {108, -3},
        {26, -2},
        {1, -1},
        {0, 0},
        {2, 1},
        {25, 2},
        {99, 3},
        {439, 4},
        {395, 5},
        {196, 100},  
        {30, 110}, 
        {55, 120}, 
        {7, 150}
    };

    void print_histograms(){
        std::cerr << "delta histogram" << std::endl;
        int sum_delta {0};
        int neg_x {};
        int pos_x {};
        for (const auto& [value, frequency] : delta_frequency){
            if(value < -5)
                neg_x += frequency;
            else if(value > 5)
                pos_x += frequency;
            sum_delta+=frequency;
        }
        for(int value = -5; value <= 5; value++){
            std::cerr << value << " " << delta_frequency[value] << std::endl;
        }
        std::cerr << "neg_x" << neg_x << std::endl;
        std::cerr << "neg_y" << pos_x << std::endl;
        std::cerr << "sum delta " << sum_delta << std::endl;
        std::cerr << "------------------------------------------------" << std::endl;
        int sum_RLE {0};
        std::cerr << "RLE histogram" << std::endl;
        for (const auto& [value, frequency] : RLE_frequency){
            std::cerr << "RLE: " << value+1 << " with frequency " << frequency << std::endl;
            sum_RLE+=frequency;
        }
        std::cerr << "sum RLE " << sum_RLE << std::endl;
    }

    void huffman_histogram(){
        std::cerr << "symbol usage stats" << std::endl;
        int sum_delta {0};
        int neg_x {};
        int pos_x {};
        for (const auto& [symbol, frequency] : huffman_frequency){
            std::cerr << "symbol [" << symbol << "] --> " << frequency << std::endl;
        }
    }

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

  
        if(idx == 64)
            RLE_frequency[1000]++;
        else
            RLE_frequency[count]++;
        return count;
    }

    // void push_motion_vector_RLE(OutputBitStream& stream, const std::vector<int>& mv){
    //     // push first vector normally
    //     push_value_n(stream, mv.at(0), 4);
    //     push_value_n(stream, mv.at(1), 4);

    //     u32 idx = 2;
    //     while(idx < mv.size()){
    //         if(mv.at(idx) != 0){
    //             push_delta_value(stream, mv.at(idx++));
    //         }else{
    //             u32 count = idx +1;
    //             while(count < mv.size() && mv.at(count) == 0 && count-idx < 8){
    //                 count++;
    //             }
    //             u32 num_zeros = count - idx -1;
    //             // push initial 0 then count
    //             stream.push_bit(0);
    //             stream.push_bits(num_zeros, 3);
    //             idx = count;
    //         }
    //     }
    // }

    Array64 quantized_to_delta(const Array64& quantized){
        Array64 delta_values;
        delta_values.at(0) = quantized.at(0);
        delta_values.at(1) = quantized.at(1);

        for(u32 idx = 2; idx < 64; idx++){
            delta_values.at(idx) = quantized.at(idx) - quantized.at(idx-1);
        }
        return delta_values;
    }

    void push_symbol_huffman(OutputBitStream& stream, int symbol){
        huffman_frequency[symbol]++;
        // std::cerr << "push_symbol_huffman " << std::endl;
        u32 num_bits = symbol_length[symbol];
        u32 code_bits = symbol_encoding[symbol];

        for(u32 idx = 0; idx < num_bits; idx++)
            stream.push_bit(code_bits >> (num_bits - idx - 1) & 1);
    }

    void push_unary(OutputBitStream& stream, u32 value){
        // std::cerr << "push_unary value " << value << std::endl;
        for(u32 idx = 0; idx < value; idx++)
            stream.push_bit(1);
        stream.push_bit(0);
    }

    u32 count_RLE_zeros(const Array64& array, u32 start){
        // std::cerr << "count_RLE_zeros" << std::endl;
        u32 idx = start; 
        u32 count = 0;
        while(idx < 64 && array.at(idx) == 0){
            count++;
            idx++;
        }
        return count;
    }

    void push_quantized_array_delta(OutputBitStream& stream, const Array64& array){
        // std::cerr << "push quantized array delta " << std::endl;
        Array64 delta_values = quantized_to_delta(array);
        for(double delta : delta_values)
            delta_frequency[delta]++;

        // Send first 2 values as normal
        push_value(stream, delta_values.at(0));
        push_value(stream, delta_values.at(1));

        // Use huffman codes to send over delta values
        u32 idx = 2;
        while(idx < 64){
            if(delta_values.at(idx) < -5){
                push_symbol_huffman(stream, -100);
                push_unary(stream, -1 * delta_values.at(idx++));
            }else if(delta_values.at(idx) > 5){
                push_symbol_huffman(stream, 100);
                push_unary(stream, delta_values.at(idx++));
            }else if(delta_values.at(idx) != 0){
                push_symbol_huffman(stream, delta_values.at(idx++));
            }else{
                // count the run length of zero
                u32 num_zeros = count_RLE_zeros(delta_values, idx);
                idx += num_zeros;

                if(idx == 64){  
                    push_symbol_huffman(stream, 150);   // the rest zero
                    return;
                }
                while(num_zeros >= 8){
                    push_symbol_huffman(stream, 120);   // 8 zeros
                    num_zeros -= 8;
                }
                while(num_zeros >= 4){
                    push_symbol_huffman(stream, 110);   // 4 zeros
                    num_zeros -= 4;
                }
                while(num_zeros > 0){
                    push_symbol_huffman(stream, 0);     // single zero
                    num_zeros --;
                }
            }
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

    int read_symbol_huffman(InputBitStream& stream){
        u32 value = 0;
        u32 iter = 0;
        while(iter < 100){
            value = (value << 1) | stream.read_bit();
            auto symbol_ref = encoding_symbol.find(value);
            if (symbol_ref != encoding_symbol.end()){
                return symbol_ref->second;      
            }
        }
        std::cerr<<"inifinite loop line 377"<<std::endl;
    }

    int read_unary(InputBitStream& stream){
        int value = 0;
        while(stream.read_bit())
            value++;
        return value;
    }

    Array64 read_quantized_array_delta(InputBitStream& stream){
        Array64 delta_values;

        // Read first 2 as normal
        delta_values.at(0) = read_value(stream);
        delta_values.at(1) = read_value(stream);

        // Read the rest with huffman codes 
        u32 idx = 2;
        while(idx < 64){
            int curr_symbol = read_symbol_huffman(stream);
            if(curr_symbol == -100){
                delta_values.at(idx++) = -1 * read_unary(stream);
            }else if(curr_symbol == 100){
                delta_values.at(idx++) = 1 * read_unary(stream);
            }else if(curr_symbol == 110){
                for(u32 i = 0; i < 4; i++)
                    delta_values.at(idx++) = 0;
            }else if(curr_symbol == 120){
                for(u32 i = 0; i < 8; i++)
                    delta_values.at(idx++) = 0;
            }else if(curr_symbol == 150){
                while(idx < 64)
                    delta_values.at(idx++) = 0;
            }else{
                delta_values.at(idx++) = curr_symbol;
            }
        }

        Array64 quantized = delta_to_quantized(delta_values);
        return quantized;
    }

}