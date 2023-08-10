#include "discrete_cosine_transform.hpp"
#include <iostream>

namespace dct{

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

    // Returns the 8x8 block of delta values of block1 - block2
    Block8x8 get_delta_block(const Block8x8& block1, const Block8x8& block2){
        Block8x8 delta;
        for (u32 r = 0; r < 8; r++)
            for(u32 c = 0; c < 8; c++)
                delta.at(r).at(c) = block1.at(r).at(c) - block2.at(r).at(c);
        return delta;
    }

    // Returns the 8x8 block of adding the delta values to the block
    Block8x8 add_delta_block(const Block8x8& block, const Block8x8& delta){
        Block8x8 result;
        for (u32 r = 0; r < 8; r++)
            for(u32 c = 0; c < 8; c++)
                result.at(r).at(c) = block.at(r).at(c) + delta.at(r).at(c);
        return result;
    }

    /* ----- Compressor Functions ----- */

    // given a color channel partitions into 8x8 blocks and adds blocks to vector in row major order
    void partition_channel(std::vector<Block8x8>& blocks, u32 height, u32 width, const std::vector<std::vector<unsigned char>>& channel){
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
        Direction dir = right;
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
        Direction dir = right;
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
    void undo_partition_channel(const std::vector<Block8x8>& blocks, u32 height, u32 width, std::vector<std::vector<unsigned char>>& channel){
        u32 idx = 0;
        for(u32 r = 0; r < height; r+=8){
            for(u32 c = 0; c < width; c+=8){
                const Block8x8& current_block = blocks.at(idx++);
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