#include <iostream>
#include <vector>
#include <cmath>
#include "discrete_cosine_transform.hpp"

using u32 = std::uint32_t;

void print_block(const Block8x8& matrix){
    for(u32 r = 0; r < 8; r++){
        std::cout << "{ ";
        for(u32 c = 0; c < 8; c++){
            std::cout << matrix.at(r).at(c) << ", ";
        }
        std::cout << "}" << std::endl;
    }
}




int main(){
    Block8x8 c_matrix = create_c_matrix(8);
    // std::cout << "----" << std::endl;
    // print_block(c_matrix);

    const u32 width = 8;
    const u32 height = 8;

    std::array<std::array<double, width>, height> practice;
    u32 counter = 1;
    for(u32 r = 0; r < height; r++){
        for(u32 c = 0; c < width; c++){
            practice.at(r).at(c) = counter++; 
        }
    }

    Block8x8 mult = multiply_block(practice, practice);
    std::cout << "----" << std::endl;
    print_block(mult);

    // Block8x8 current_block;
    // for(u32 r = 0; r < height; r+=8){
    //     for(u32 c = 0; c < width; c+=8){
    //         std::cout << "[" << r << ", " <<c << "]" <<std::endl;
    //         for(u32 sub_r = 0; sub_r < 8; sub_r++){
    //             for(u32 sub_c = 0; sub_c < 8; sub_c++){
    //                 if((r+sub_r) >= height && (c+sub_c) < width){
    //                     current_block.at(sub_r).at(sub_c) = practice.at(height-1).at(c+sub_c);;
    //                 }else if((c+sub_c) >= width && (r+sub_r) < height){
    //                     current_block.at(sub_r).at(sub_c) = practice.at(r+sub_r).at(width-1);
    //                 }else if((r+sub_r) >= height && (c+sub_c) >= width){
    //                     current_block.at(sub_r).at(sub_c) = practice.at(height-1).at(width-1);
    //                 }else{
    //                     current_block.at(sub_r).at(sub_c) = practice.at(r+sub_r).at(c+sub_c);
    //                 }
    //             }
    //         }
    //         print_block(current_block, 8, 8);
    //         std::cout << "-------" << std::endl;
    //     }
    // }

    return 0;
}