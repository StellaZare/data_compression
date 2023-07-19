#include <iostream>
#include <vector>
#include <cmath>
#include "discrete_cosine_transform.hpp"

using u32 = std::uint32_t;

Block8x8 create_c_matrix(double n){
    Block8x8 c_matrix {};
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

void print_matrix(const Block8x8& matrix, u32 n){
    for(u32 r = 0; r < n; r++){
        std::cout << "{";
        for(u32 c = 0; c <n; c++){
            std::cout << matrix.at(r).at(c) << ", ";
        }
        std::cout << "}" << std::endl;
    }
}

int main(){
    Block8x8 c_matrix = create_c_matrix(8);
    std::cout << "----" << std::endl;
    print_matrix(c_matrix, 8);

    return 0;
}