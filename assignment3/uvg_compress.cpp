/* uvg_compress.cpp

   Starter code for Assignment 3 (in C++). This program
    - Reads an input image in BMP format
     (Using bitmap_image.hpp, originally from 
      http://partow.net/programming/bitmap/index.html)
    - Transforms the image from RGB to YCbCr (i.e. "YUV").
    - Downscales the Cb and Cr planes by a factor of two
      (producing the same resolution that would result
       from 4:2:0 subsampling, but using interpolation
       instead of ignoring some samples)
    - Writes each colour plane (Y, then Cb, then Cr)
      in 8 bits per sample to the output file.

   B. Bird - 2023-07-03
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>
#include "output_stream.hpp"
#include "bitmap_image.hpp"
#include "uvg_common.hpp"
#include "discrete_cosine_transform.hpp"


//A simple downscaling algorithm using averaging.
std::vector<std::vector<unsigned char> > scale_down(std::vector<std::vector<unsigned char> > source_image, unsigned int source_width, unsigned int source_height, int factor){

    unsigned int scaled_height = (source_height+factor-1)/factor;
    unsigned int scaled_width = (source_width+factor-1)/factor;

    //Note that create_2d_vector automatically initializes the array to all-zero
    auto sums = create_2d_vector<unsigned int>(scaled_height,scaled_width);
    auto counts = create_2d_vector<unsigned int>(scaled_height,scaled_width);

    for(unsigned int y = 0; y < source_height; y++)
        for (unsigned int x = 0; x < source_width; x++){
            sums.at(y/factor).at(x/factor) += source_image.at(y).at(x);
            counts.at(y/factor).at(x/factor)++;
        }

    auto result = create_2d_vector<unsigned char>(scaled_height,scaled_width);
    for(unsigned int y = 0; y < scaled_height; y++)
        for (unsigned int x = 0; x < scaled_width; x++)
            result.at(y).at(x) = (unsigned char)((sums.at(y).at(x)+0.5)/counts.at(y).at(x));
    return result;
}


int main(int argc, char** argv){

    if (argc < 4){
        std::cerr << "Usage: " << argv[0] << " <low/medium/high> <input BMP> <output file>" << std::endl;
        return 1;
    }
    std::string quality{argv[1]};
    std::string input_filename {argv[2]};
    std::string output_filename {argv[3]};

    bitmap_image input_image {input_filename};

    unsigned int height = input_image.height();
    unsigned int width = input_image.width();

    std::cout << "height: " << height << " width: " << width << std::endl;

    //Read the entire image into a 2d array of PixelRGB objects 
    //(Notice that height is the outer dimension, so the pixel at coordinates (x,y) 
    // must be accessed as imageRGB.at(y).at(x)).
    std::vector<std::vector<PixelYCbCr>> imageYCbCr = create_2d_vector<PixelYCbCr>(height,width);
    for(unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            auto [r,g,b] = input_image.get_pixel(x,y);
            PixelRGB rgb_pixel {r,g,b};
            imageYCbCr.at(y).at(x) = rgb_pixel.to_ycbcr();            
        }
    }

    // Separate Y Cb Cr channels
    auto Y_matrix = create_2d_vector<unsigned char>(height, width);
    auto Cb_matrix = create_2d_vector<unsigned char>(height, width);
    auto Cr_matrix = create_2d_vector<unsigned char>(height, width);
    for(unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            Y_matrix.at(y).at(x) = imageYCbCr.at(y).at(x).Y; 
            Cb_matrix.at(y).at(x) = imageYCbCr.at(y).at(x).Cb; 
            Cr_matrix.at(y).at(x) = imageYCbCr.at(y).at(x).Cr; 
        }
    }

    // Downscale Cb and Cr color channels
    unsigned int scaled_height = (height+2-1)/2;
    unsigned int scaled_width = (width+2-1)/2;
    auto Cb_scaled = scale_down(Cb_matrix,width,height,2);
    auto Cr_scaled = scale_down(Cr_matrix,width,height,2);

    // Partition colow channels into 8x8 blocks and fill
    std::vector<Block8x8> Y_blocks, Cb_blocks, Cr_blocks;
    dct::partition_channel(Y_blocks, height, width, Y_matrix);
    dct::partition_channel(Cb_blocks, scaled_height, scaled_width, Cb_scaled);
    dct::partition_channel(Cr_blocks, scaled_height, scaled_width, Cr_scaled);

    // std::cout << "Y blocks: " << Y_blocks.size() << std::endl;
    // std::cout << "Cb blocks: " << Cb_blocks.size() << std::endl;
    // std::cout << "Cr blocks: " << Cr_blocks.size() << std::endl;

    // for(const Block8x8& b: Y_blocks){
    //     std::cout << "------" << std::endl;
    //     dct::print_block8x8(b);
    // }

    // std::ofstream output_file{output_filename,std::ios::binary};
    // OutputBitStream output_stream {output_file};

    // output_stream.push_u32(height);
    // output_stream.push_u32(width);

    // //Write the Y values 
    // for(unsigned int y = 0; y < height; y++)
    //     for (unsigned int x = 0; x < width; x++)
    //         output_stream.push_byte(imageYCbCr.at(y).at(x).Y);

    // //Extract the Cb plane into its own array 
    // auto Cb = create_2d_vector<unsigned char>(height,width);
    // for(unsigned int y = 0; y < height; y++)
    //     for (unsigned int x = 0; x < width; x++)
    //         Cb.at(y).at(x) = imageYCbCr.at(y).at(x).Cb;
    // auto Cb_scaled = scale_down(Cb,width,height,2);

    // //Extract the Cr plane into its own array 
    // auto Cr = create_2d_vector<unsigned char>(height,width);
    // for(unsigned int y = 0; y < height; y++)
    //     for (unsigned int x = 0; x < width; x++)
    //         Cr.at(y).at(x) = imageYCbCr.at(y).at(x).Cr;
    // auto Cr_scaled = scale_down(Cr,width,height,2);

    // //Write the Cb values 
    // for(unsigned int y = 0; y < (height+1)/2; y++)
    //     for (unsigned int x = 0; x < (width+1)/2; x++)
    //         output_stream.push_byte(Cb_scaled.at(y).at(x));

    // //Write the Cr values 
    // for(unsigned int y = 0; y < (height+1)/2; y++)
    //     for (unsigned int x = 0; x < (width+1)/2; x++)
    //         output_stream.push_byte(Cr_scaled.at(y).at(x));


    // output_stream.flush_to_byte();
    // output_file.close();

    return 0;
}