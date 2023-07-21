/* uvg_decompress.cpp

   Starter code for Assignment 3 (in C++). This program
    - Reads a height/width value from the input file
    - Reads YCbCr data from the file, with the Y plane
      in full w x h resolution and the other two planes
      in half resolution.
    - Upscales the Cb and Cr planes to full resolution and
      transforms them to RGB.
    - Writes the result as a BMP image
     (Using bitmap_image.hpp, originally from 
      http://partow.net/programming/bitmap/index.html)

   B. Bird - 2023-07-03
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include <cmath>
#include "input_stream.hpp"
#include "bitmap_image.hpp"
#include "uvg_common.hpp"
#include "discrete_cosine_transform.hpp"
#include "bit_stream.hpp"


int main(int argc, char** argv){
    if (argc < 3){
        std::cerr << "Usage: " << argv[0] << " <input file> <output BMP>" << std::endl;
        return 1;
    }
    std::string input_filename {argv[1]};
    std::string output_filename {argv[2]};


    std::ifstream input_file{input_filename,std::ios::binary};
    InputBitStream input_stream {input_file};

    dct::Quality quality;
    u16 height, width;
    stream::readHeader(input_stream, quality, height, width);

    // calculate number of Y_matrix blocks expected
    u16 Y_blocks_wide = (width%8 == 0) ? width/8 : (width/8)+1;
    u16 Y_blocks_high = (height%8 == 0) ? height/8 : (height/8)+1;
    u16 num_Y_blocks = Y_blocks_wide * Y_blocks_high;

    // calculate number of Cb and Cr blocks expected
    u16 scaled_height = (height+1)/2;
    u16 scaled_width = (width+1)/2; 
    u16 C_blocks_wide = (scaled_width%8 == 0) ? scaled_width/8 : (scaled_width/8)+1;
    u16 C_blocks_high = (scaled_height%8 == 0) ? scaled_height/8 : (scaled_height/8)+1;
    u16 num_C_blocks = C_blocks_wide * C_blocks_high;

    // read blocks for each color channel in row major order
    std::vector<Block8x8> Y_blocks, Cb_blocks, Cr_blocks;
    for(u16 blocks_read = 0; blocks_read < num_Y_blocks; blocks_read++){
        Block8x8 block = dct::array_to_block(stream::readQuantizedArray(input_stream));
        block = dct::unquantize_block(block, quality, dct::luminance);
        block = dct::get_inverse_dct(block);
        Y_blocks.push_back(block);
    }
    for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
        Block8x8 block = dct::array_to_block(stream::readQuantizedArray(input_stream));
        block = dct::get_inverse_dct( dct::unquantize_block(block, quality, dct::chrominance) );
        Cb_blocks.push_back(block);
    }
    for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
        Block8x8 block = dct::array_to_block(stream::readQuantizedArray(input_stream));
        block = dct::unquantize_block(block, quality, dct::chrominance);
        block = dct::get_inverse_dct( block );
        Cr_blocks.push_back(block);
    }

    // undo particitioning of channels into 8x8 blocks
    auto Y_matrix = create_2d_vector<unsigned char>(height,width);
    auto Cb_scaled = create_2d_vector<unsigned char>(scaled_height,scaled_width);
    auto Cr_scaled = create_2d_vector<unsigned char>(scaled_height,scaled_width);

    dct::undo_partition_channel(Y_blocks, height, width, Y_matrix);
    dct::undo_partition_channel(Cb_blocks, scaled_height, scaled_width, Cb_scaled);
    dct::undo_partition_channel(Cr_blocks, scaled_height, scaled_width, Cr_scaled);

    auto imageYCbCr = create_2d_vector<PixelYCbCr>(height,width);
    for (unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            imageYCbCr.at(y).at(x) = {
                Y_matrix.at(y).at(x),
                Cb_scaled.at(y/2).at(x/2),
                Cr_scaled.at(y/2).at(x/2)
            };
        }
    }

    // dct::print_image_YCbCr(imageYCbCr, height, width);
    
    input_stream.flush_to_byte();
    input_file.close();

    bitmap_image output_image {width,height};

    for (unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            auto pixel_rgb = imageYCbCr.at(y).at(x).to_rgb();
            auto [r,g,b] = pixel_rgb;
            output_image.set_pixel(x,y,r,g,b);
        }
    }
    
    output_image.save_image(output_filename);

    return 0;
}