#include <iostream>
#include <vector>
#include <cmath>
#include "discrete_cosine_transform.hpp"
#include "uvg_common.hpp"


void write_delta(u16 num){
    for(u16 i = 0; i < num; i++){
        std::cout << 1;
    }
    std::cout << 0 << std::endl;
}

void write_array(std::array<int, 8> array){
    for(u32 idx = 0; idx < 2; idx++){
        int num = array.at(idx);
        bool sign = (num < 0) ? 1 : 0;
        std::cout << "(" << sign << ")";
        // value
        num = (num < 0) ? -1*num : num;
        std::cout << num << " ";
    }
    for(u32 idx = 2; idx < 8; idx++){
        int delta = array.at(idx-1) - array.at(idx);
        if (delta < 0){
            std::cout << "10";
            write_delta(delta*-1);
        }else{
            std::cout << "11";
            write_delta(delta);
        }  
    }
}

u16 read_delta(){
    u16 value = 0;
    u16 bit;
    std::cin >> bit;
    while(bit == 1){
        value++;
        std::cin >> bit;
    }
    return value;
}

int main(){

    // std::array<int, 8> arr;
    // arr.at(0) = 35;
    // arr.at(1) = -15;
    // arr.at(2) = 3;
    // arr.at(3) = 0;
    // arr.at(4) = 3;
    // arr.at(5) = 0;
    // arr.at(6) = 5;
    // arr.at(7) = -5;

    // write_array(arr);

    std::cout << read_delta() << std::endl;
    
    // Array64 array = dct::block_to_array(dct::quantization_order);
    // dct::print_array(array);

    // Block8x8 block = dct::array_to_block(array);
    // dct::print_block(block);

    // u32 height = 30;
    // u32 width = 15;
    // u32 count = 1;

    // auto matrix = create_2d_vector<unsigned char>(height, width);
    // for(u32 r = 0; r < height; r++){
    //     for(u32 c = 0; c < width; c++){
    //         matrix.at(r).at(c) = count++;
    //     }
    // }

    // std::vector<Block8x8> blocks;
    // dct::partition_channel(blocks, height, width, matrix);
    // for(const Block8x8& b : blocks){
    //     std::cout << "------" <<std::endl;
    //     dct::print_block(b);
    // }

    // auto after = create_2d_vector<unsigned char>(height, width);
    // std::cout << "matrix ---------" <<std::endl;
    // for(u32 r = 0; r < height; r++){
    //     for(u32 c = 0; c < width; c++){
    //         std::cout << matrix.at(r).at(c) << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // dct::undo_partition_channel(blocks, height, width, after);
    // std::cout << "matrix ---------" <<std::endl;
    // for(u32 r = 0; r < height; r++){
    //     for(u32 c = 0; c < width; c++){
    //         std::cout << matrix.at(r).at(c) << " ";
    //     }
    //     std::cout << std::endl;
    // }

    return 0;
}