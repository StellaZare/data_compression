/* uvid_decompress.cpp
   CSC 485B/578B - Data Compression - Summer 2023

   Starter code for Assignment 4
   
   This placeholder code reads the (basically uncompressed) data produced by
   the uvid_compress starter code and outputs it in the uncompressed 
   YCbCr (YUV) format used for the sample video input files. To play the 
   the decompressed data stream directly, you can pipe the output of this
   program to the ffplay program, with a command like 

     ffplay -f rawvideo -pixel_format yuv420p -framerate 30 -video_size 352x288 - 2>/dev/null
   (where the resolution is explicitly given as an argument to ffplay).

   B. Bird - 2023-07-08
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include <tuple>
#include "input_stream.hpp"
#include "yuv_stream.hpp"
#include "discrete_cosine_transform.hpp"
#include "stream.hpp"
#include "helper.hpp"


int main(int argc, char** argv){

    //Note: This program must not take any command line arguments. (Anything
    //      it needs to know about the data must be encoded into the bitstream)
    
    InputBitStream input_stream {std::cin};

    dct::Quality quality;
    u16 height, width;
    stream::read_header(input_stream, quality, height, width);

    YUVStreamWriter writer {std::cout, width, height};

    // calculate number of Y_matrix blocks expected
    u16 Y_blocks_wide = (width%8 == 0) ? width/8 : (width/8)+1;
    u16 Y_blocks_high = (height%8 == 0) ? height/8 : (height/8)+1;
    u16 num_Y_blocks = Y_blocks_wide * Y_blocks_high;

    // calculate number of Cb and Cr blocks expected
    u16 scaled_height = height/2;
    u16 scaled_width = width/2; 
    u16 C_blocks_wide = (scaled_width%8 == 0) ? scaled_width/8 : (scaled_width/8)+1;
    u16 C_blocks_high = (scaled_height%8 == 0) ? scaled_height/8 : (scaled_height/8)+1;
    u16 num_C_blocks = C_blocks_wide * C_blocks_high;

    while (input_stream.read_byte()){
        YUVFrame420& frame = writer.frame();
        // read blocks for each color channel in row major order
        std::vector<Block8x8> Y_blocks, Cb_blocks, Cr_blocks;
        for(u16 blocks_read = 0; blocks_read < num_Y_blocks; blocks_read++){
            Block8x8 block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            block = dct::unquantize_block(block, quality, dct::luminance);
            block = dct::get_inverse_dct(block);
            Y_blocks.push_back(block);
        }
        for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
            Block8x8 block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            block = dct::get_inverse_dct( dct::unquantize_block(block, quality, dct::chrominance) );
            Cb_blocks.push_back(block);
        }
        for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
            Block8x8 block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            block = dct::unquantize_block(block, quality, dct::chrominance);
            block = dct::get_inverse_dct( block );
            Cr_blocks.push_back(block);
        }

        auto Y_matrix = helper::create_2d_vector<unsigned char>(height,width);
        auto Cb_matrix = helper::create_2d_vector<unsigned char>(height/2 ,width/2);
        auto Cr_matrix = helper::create_2d_vector<unsigned char>(height/2 ,width/2);

        dct::undo_partition_channel(Y_blocks, height, width, Y_matrix);
        dct::undo_partition_channel(Cb_blocks, height/2 ,width/2, Cb_matrix);
        dct::undo_partition_channel(Cr_blocks, height/2 ,width/2, Cr_matrix);

        // Create and fill frame
        writer.write_frame();
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                frame.Y(x,y) = Y_matrix.at(y).at(x);
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++){
                frame.Cb(x,y) = Cb_matrix.at(y).at(x);
                frame.Cr(x,y) = Cr_matrix.at(y).at(x);
            }
    }

    return 0;
}