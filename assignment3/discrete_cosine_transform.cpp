#include <iostream>
#include <vector>
#include <cmath>
#include "discrete_cosine_transform.hpp"
#include "uvg_common.hpp"

int main(){
    
    // Array64 array = dct::block_to_array(dct::quantization_order);
    // dct::print_array(array);

    // Block8x8 block = dct::array_to_block(array);
    // dct::print_block(block);

    u32 height = 30;
    u32 width = 15;
    u32 count = 1;

    auto matrix = create_2d_vector<unsigned char>(height, width);
    for(u32 r = 0; r < height; r++){
        for(u32 c = 0; c < width; c++){
            matrix.at(r).at(c) = count++;
        }
    }

    std::vector<Block8x8> blocks;
    dct::partition_channel(blocks, height, width, matrix);
    // for(const Block8x8& b : blocks){
    //     std::cout << "------" <<std::endl;
    //     dct::print_block(b);
    // }

    auto after = create_2d_vector<unsigned char>(height, width);
    std::cout << "matrix ---------" <<std::endl;
    for(u32 r = 0; r < height; r++){
        for(u32 c = 0; c < width; c++){
            std::cout << matrix.at(r).at(c) << " ";
        }
        std::cout << std::endl;
    }
    dct::undo_partition_channel(blocks, height, width, after);
    std::cout << "matrix ---------" <<std::endl;
    for(u32 r = 0; r < height; r++){
        for(u32 c = 0; c < width; c++){
            std::cout << matrix.at(r).at(c) << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}