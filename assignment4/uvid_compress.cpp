/* uvid_compress.cpp
   CSC 485B/578B - Data Compression - Summer 2023

   Starter code for Assignment 4

   Reads video data from stdin in uncompresed YCbCr (YUV) format 
   (With 4:2:0 subsampling). To produce this format from 
   arbitrary video data in a popular format, use the ffmpeg
   tool and a command like 

     ffmpeg -i videofile.mp4 -f rawvideo -pixel_format yuv420p - 2>/dev/null | ./this_program <width> height>

   Note that since the width/height of each frame is not encoded into the raw
   video stream, those values must be provided to the program as arguments.

   B. Bird - 2023-07-08
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include <tuple>
#include "output_stream.hpp"
#include "stream.hpp"
#include "yuv_stream.hpp"
#include "discrete_cosine_transform.hpp"
#include "helper.hpp"


int main(int argc, char** argv){

    if (argc < 4){
        std::cerr << "Usage: " << argv[0] << " <width> <height> <low/medium/high>" << std::endl;
        return 1;
    }

    // Parse command line arguments
    u16 width = std::stoi(argv[1]);
    u16 height = std::stoi(argv[2]);
    dct::Quality quality = helper::get_quality(argv[3]);
    if(quality == dct::Quality::ERROR){
        std::cerr << "Usage: " << argv[0] << " <width> <height> <low/medium/high>" << std::endl;
        return 1;
    }

    YUVStreamReader reader {std::cin, width, height};
    OutputBitStream output_stream {std::cout};

    stream::push_header(output_stream, quality, height, width);

    YUVFrame420 prev_frame {width, height};
    u32 num_frames = 0;

    while (reader.read_next_frame()){
        output_stream.push_byte(1); // 1=Iframe or 2=Pframe
        
        YUVFrame420& active_frame = reader.frame();

        // Separate Y Cb and Cr channels
        auto Y_matrix = helper::create_2d_vector<unsigned char>(height, width);
        auto Cb_matrix = helper::create_2d_vector<unsigned char>(height/2, width/2);
        auto Cr_matrix = helper::create_2d_vector<unsigned char>(height/2, width/2);

        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                Y_matrix.at(y).at(x) = active_frame.Y(x,y);
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++){
                Cb_matrix.at(y).at(x) = active_frame.Cb(x,y);
                Cr_matrix.at(y).at(x) = active_frame.Cr(x,y);
            }
        
        // Partition color channels into 8x8 blocks
        std::vector<Block8x8> Y_blocks, Cb_blocks, Cr_blocks;
        dct::partition_channel(Y_blocks, height, width, Y_matrix);
        dct::partition_channel(Cb_blocks, height/2, width/2, Cb_matrix);
        dct::partition_channel(Cr_blocks, height/2, width/2, Cr_matrix);

        // To store uncompressed blocks 
        std::vector<Block8x8> Y_uncompressed, Cb_uncompressed, Cr_uncompressed;

        for(Block8x8& curr_block : Y_blocks){
            // Take the DCT and quantize
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(curr_block), quality, dct::luminance);
            // Push in array format
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            // Unquantize and take the inverse DCT
            Y_uncompressed.push_back(helper::simulate_decompressor(quantized_block, quality, dct::luminance));
        }
        for(Block8x8& curr_block : Cb_blocks){
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(curr_block), quality, dct::chrominance);
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            Cb_uncompressed.push_back(helper::simulate_decompressor(quantized_block, quality, dct::chrominance));
        }
        for(Block8x8& curr_block : Cr_blocks){
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(curr_block), quality, dct::chrominance);
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            Cr_uncompressed.push_back(helper::simulate_decompressor(quantized_block, quality, dct::chrominance));
        }
        
        // Update the prev_frame as seen by the decompressor
        prev_frame = helper::reconstruct_frame(height, width, Y_uncompressed, Cb_uncompressed, Cr_uncompressed);
    }

    output_stream.push_byte(0); //Flag to indicate end of data
    output_stream.flush_to_byte();

    return 0;
}