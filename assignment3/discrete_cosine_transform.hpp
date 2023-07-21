#ifndef DISCRETE_COSINE_TRANSFORM
#define DISCRETE_COSINE_TRANSFORM

#include <vector>
#include <cmath>
#include <array>
#include "uvg_common.hpp"

using Block8x8 = std::array<std::array<double, 8>, 8>;
using Array64  = std::array<int, 64>;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

namespace dct{

    // enum for quality level - used in quantize_block()
    enum Quality {
        low = 0,        // 2 * quantize matrix   
        medium,         // 1 * quantize matrix     
        high            // 0.5 * quantize matrix    
    };

    // enum for incrementation direction - used in get_direction() and block_to_array()
    enum Direction {
        right = 0,
        down,
        down_left,
        up_right
    };

    // the result of running create_c_matrix()  
    const Block8x8 c_matrix {{
        {0.353553,  0.353553,   0.353553,   0.353553,   0.353553,   0.353553,   0.353553,   0.353553    },
        {0.490393,  0.415735,   0.277785,   0.0975452,  -0.0975452, -0.277785,  -0.415735,  -0.490393   },
        {0.46194,   0.191342,   -0.191342,  -0.46194,   -0.46194,   -0.191342,  0.191342,   0.46194     },
        {0.415735,  -0.0975452, -0.490393,  -0.277785,  0.277785,   0.490393,   0.0975452,  -0.415735   },
        {0.353553,  -0.353553,  -0.353553,  0.353553,   0.353553,   -0.353553,  -0.353553,  0.353553    },
        {0.277785,  -0.490393,  0.0975452,  0.415735,   -0.415735,  -0.0975452, 0.490393,   -0.277785   },
        {0.191342,  -0.46194,   0.46194,    -0.191342,  -0.191342,  0.46194,    -0.46194,   0.191342    },
        {0.0975452, -0.277785,  0.415735,   -0.490393,  0.490393,   -0.415735,  0.277785,   -0.0975452  }
    }};

    // the result of running transpose_block() from discrete_cosine_transfrom.cpp 
    const Block8x8 c_matrix_transpose {{
        {0.353553, 0.490393,   0.46194,   0.415735,   0.353553,  0.277785,   0.191342,  0.0975452   },
        {0.353553, 0.415735,   0.191342,  -0.0975452, -0.353553, -0.490393,  -0.46194,  -0.277785   },
        {0.353553, 0.277785,   -0.191342, -0.490393,  -0.353553, 0.0975452,  0.46194,   0.415735    },
        {0.353553, 0.0975452,  -0.46194,  -0.277785,  0.353553,  0.415735,   -0.191342, -0.490393   },
        {0.353553, -0.0975452, -0.46194,  0.277785,   0.353553,  -0.415735,  -0.191342, 0.490393    },
        {0.353553, -0.277785,  -0.191342, 0.490393,   -0.353553, -0.0975452, 0.46194,   -0.415735   },
        {0.353553, -0.415735,  0.191342,  0.0975452,  -0.353553, 0.490393,   -0.46194,  0.277785    },
        {0.353553, -0.490393,  0.46194,   -0.415735,  0.353553,  -0.277785,  0.191342,  -0.0975452  }
    }};

    // quantization matrix used by JPEG - from lecture slides
    const Block8x8 luminance {{
        {16, 11, 10, 16, 24,  40,  51,  61},
        {12, 12, 14, 19, 26,  58,  60,  55},
        {14, 13, 16, 24, 40,  57,  69,  56},
        {14, 17, 22, 29, 51,  87,  80,  62},
        {18, 22, 37, 56, 68,  109, 103, 77},
        {24, 35, 55, 64, 81,  104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99}
    }};

    // quantization matrix used by JPEG - from lecture slides
    const Block8x8 chrominance {{
        {17, 18, 24, 47, 99, 99, 99, 99},
        {18, 21, 26, 66, 99, 99, 99, 99},
        {24, 26, 56, 99, 99, 99, 99, 99},
        {47, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
    }};

    const Block8x8 quantization_order {{
        {0,  1,  5,  6,  14, 15, 27, 28},
        {2,  4,  7,  13, 16, 26, 29, 42},
        {3,  8,  12, 17, 25, 30, 41, 43},
        {9,  11, 18, 24, 31, 40, 44, 53},
        {10, 19, 23, 32, 39, 45, 52, 54},
        {20, 22, 33, 38, 46, 51, 55, 60},
        {21, 34, 37, 47, 50, 56, 59, 61},
        {35, 36, 48, 49, 57, 58, 62, 63}
    }};

    /* ----- Block Operations ----- */

    // returns the c_matrix for n = 8
    Block8x8 create_c_matrix(){
        Block8x8 c_matrix {};
        double n = 8;
        double root_1_over_n = std::sqrt(1/n);
        double root_2_over_n = std::sqrt(2/n);

        for(u32 r = 0; r < n; r++){
            for(u32 c = 0; c < n; c++){
                if(r == 0){
                    c_matrix.at(r).at(c) = root_1_over_n;
                }else{
                    double angle = ((2*c+1) * r * M_PI)/(2*n);
                    c_matrix.at(r).at(c) = root_2_over_n * std::cos(angle);
                }
            }
        }
        return c_matrix;
    }
    
    // prints array of 64 to standard out
    void print_array(const Array64& array){
        std::cout << "-------" << std::endl;
        for(u32 idx = 0; idx < 64; idx++){
            if(idx > 0 && idx%8 == 0)
                std::cout << std::endl;
            std::cout << array.at(idx) << " ";
        }
        std::cout << std::endl;
    }
    // prints 8x8 block to standard out
    void print_block(const Block8x8& block){
        for(u32 r = 0; r < 8; r++){
            std::cout << "{ ";
            for(u32 c = 0; c < 8; c++){
                std::cout << block.at(r).at(c) << " ";
            }
            std::cout << "}" << std::endl;
        }
    }

    // prints all blocks in a block vector
    void print_blocks(const std::vector<Block8x8>& blocks){
        for(const Block8x8& b : blocks){
            std::cout << "-------" << std::endl;
            print_block(b);
        }
    }

    // print image YCbCr given the height and width 
    void print_image_YCbCr(const std::vector<std::vector<PixelYCbCr>>& image, u16 height, u16 width){
        for(u16 r = 0; r < height; r++){
            for(u16 c = 0; c < width; c++){
                std::cout << "[" << (int)image.at(r).at(c).Y << "," << (int)image.at(r).at(c).Cb << "," << (int)image.at(r).at(c).Cr << "]\t";
            }
            std::cout << std::endl;
        }
    }

    // returns the 8x8 block result of multiplying blockA by blockB
    Block8x8 multiply_block(const Block8x8& blockA, const Block8x8& blockB){
        Block8x8 result;
        for(u32 r = 0; r < 8; r++){
            for(u32 c = 0; c < 8; c++){
                double sum = 0;
                for(u32 idx = 0; idx < 8; idx++){
                    sum += (blockA.at(r).at(idx) * blockB.at(idx).at(c));
                }
                // std::cout << " = " << sum << std::endl;
                result.at(r).at(c) = sum;
            }
        }
        return result;
    }

    // returns the 8x8 block result of transposing the block
    Block8x8 transpose_block(const Block8x8& block){
        Block8x8 transpose;
        for (u32 r = 0; r < 8; r++)
            for(u32 c = 0; c < 8; c++)
                transpose.at(c).at(r) = block.at(r).at(c);
        return transpose;
    }

    /* ----- Compressor Functions ----- */

    // given a color channel partitions into 8x8 blocks and adds blocks to vector in row major order
    void partition_channel(std::vector<Block8x8>& blocks, u32 height, u32 width, std::vector<std::vector<unsigned char>> channel){
        for(u32 r = 0; r < height; r+=8){
            for(u32 c = 0; c < width; c+=8){
                Block8x8 current_block;
                // index into 8x8 sub block
                for(u32 sub_r = 0; sub_r < 8; sub_r++){
                    for(u32 sub_c = 0; sub_c < 8; sub_c++){
                        // copy element 
                        if((r+sub_r) >= height && (c+sub_c) < width){
                            current_block.at(sub_r).at(sub_c) = double(channel.at(height-1).at(c+sub_c));
                        }else if((c+sub_c) >= width && (r+sub_r) < height){
                            current_block.at(sub_r).at(sub_c) = double(channel.at(r+sub_r).at(width-1));
                        }else if((r+sub_r) >= height && (c+sub_c) >= width){
                            current_block.at(sub_r).at(sub_c) = double(channel.at(height-1).at(width-1));
                        }else{
                            current_block.at(sub_r).at(sub_c) = double(channel.at(r+sub_r).at(c+sub_c));
                        }
                    }
                }
                blocks.push_back(current_block);
            }
        }
    }
    
    // returns the dct of block A by computing [C][A][C]_transpose
    Block8x8 get_dct(const Block8x8& block){
        Block8x8 result = multiply_block(c_matrix, block);
        return multiply_block(result, c_matrix_transpose);
    }

    // returns the quantized block calculated using the provided quantization matrix at the provided quality 
    Block8x8 quantize_block(const Block8x8& block, Quality quality, const Block8x8& q_matrix){
        double multiplier;
        if(quality == low){
            multiplier = 2;
        }else if(quality == medium){
            multiplier = 1;
        }else{
            multiplier = 0.5;
        }

        Block8x8 result;
        for(u32 r = 0; r < 8; r++){
            for(u32 c = 0; c < 8; c++){
                result.at(r).at(c) = std::round(block.at(r).at(c) / (multiplier * q_matrix.at(r).at(c)) );
            }
        }
        return result;
    }

    Direction get_direction(u32 r, u32 c, Direction curr){
        u32 first = 0;
        u32 last = 7;

        if((r == first || r == last) && c%2 == 0){
            return right;
        }else if((c == first || c == last) && r%2 == 1){
            return down;
        }else if((r == first && c%2 == 1) || (c == last && r%2 == 0)){
            return down_left;
        }else if((c == first && r%2 == 0) || (r == last && c%2 == 1)){
            return up_right;
        }
        return curr;
    }

    // converts an 8x8 block to an array of 64 elements in "ideal" order
    Array64 block_to_array(const Block8x8& block){
        Direction dir;
        Array64 result;   

        u32 r = 0, c = 0, count = 0;
        while(r < 8 && c < 8){
            result.at(count++) = block.at(r).at(c);
            dir = get_direction(r, c, dir);

            if(dir == right){
                c++;
            }else if(dir == down){
                r++;
            }else if(dir == down_left){
                r++; c--;
            }else{
                r--; c++;
            }
        }
        return result;
    }

    /* ----- Decompressor Functions ----- */

    // converts an array of 64 elements in "ideal" order to an 8x8 block
    Block8x8 array_to_block(const Array64& array){
        Direction dir;
        Block8x8 result;   

        u32 r = 0, c = 0, count = 0;
        while(r < 8 && c < 8){
            result.at(r).at(c) = array.at(count++);
            dir = get_direction(r, c, dir);

            if(dir == right){
                c++;
            }else if(dir == down){
                r++;
            }else if(dir == down_left){
                r++; c--;
            }else{
                r--; c++;
            }
        }
        return result;
    }

    // returns the unquantized block calculated using the provided quantization matrix at the provided quality 
    Block8x8 unquantize_block(const Block8x8& block, Quality quality, const Block8x8& q_matrix){
        double multiplier;
        if(quality == low){
            multiplier = 2;
        }else if(quality == medium){
            multiplier = 1;
        }else{
            multiplier = 0.5;
        }

        Block8x8 result;
        for(u32 r = 0; r < 8; r++){
            for(u32 c = 0; c < 8; c++){
                result.at(r).at(c) = block.at(r).at(c) * (multiplier * q_matrix.at(r).at(c));
            }
        }
        return result;
    }

    // returns the dct of block A by computing [C][A][C]_transpose
    Block8x8 get_inverse_dct(const Block8x8& block){
        Block8x8 result = multiply_block(c_matrix_transpose, block);
        return multiply_block(result, c_matrix);
    }

    // given a vector of blocks in row major order color reconstructs the channel matrix
    void undo_partition_channel(std::vector<Block8x8>& blocks, u32 height, u32 width, std::vector<std::vector<unsigned char>>& channel){
        u32 idx = 0;
        for(u32 r = 0; r < height; r+=8){
            for(u32 c = 0; c < width; c+=8){
                Block8x8& current_block = blocks.at(idx++);
                // index into 8x8 sub block
                for(u32 sub_r = 0; sub_r < 8; sub_r++){
                    for(u32 sub_c = 0; sub_c < 8; sub_c++){
                        // copy element 
                        if( (r+sub_r) < height && (c+sub_c) < width ){
                            channel.at(r+sub_r).at(c+sub_c) = round_and_clamp_to_char(current_block.at(sub_r).at(sub_c));
                        }
                    }
                }
            }
        }
    }

}

#endif 