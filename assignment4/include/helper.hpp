#include <queue>
#include <vector>
#include <string>
#include "discrete_cosine_transform.hpp"
#include "yuv_stream.hpp"
#include "output_stream.hpp"
#include "stream.hpp"

namespace helper{
    
    /* ----- Helper Functions - written by Bill ----- */
    //Convenience function to wrap around the nasty notation for 2d vectors
    //Convenience function to wrap around the nasty notation for 2d vectors
    template<typename T>
    std::vector<std::vector<T> > create_2d_vector(unsigned int outer, unsigned int inner){
        std::vector<std::vector<T> > V {outer, std::vector<T>(inner,T() )};
        return V;
    }

    /* ----- My Helpers ----- */

    // Returns the quality enum corresponding to the input string
    // If invalid returns Quality::ERROR
    dct::Quality get_quality(std::string input_quality){
        if(input_quality == "low")
            return dct::Quality::low;
        else if(input_quality == "medium")
            return dct::Quality::medium;
        else if(input_quality == "high")
            return dct::Quality::high;
        else
            return dct::Quality::ERROR;
    }

    // // Pushes each quantized to the output stream
    // Block8x8 simulate_decompressor(const Block8x8& quantized_block, dct::Quality quality, const Block8x8& q_matrix){
    //     return dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, q_matrix));
    // }

    // YUVFrame420 reconstruct_frame(u32 height, u32 width, const std::vector<Block8x8>& Y_uncompressed, const std::vector<Block8x8>& Cb_uncompressed, const std::vector<Block8x8>& Cr_uncompressed){
    //     auto Y_matrix = helper::create_2d_vector<unsigned char>(height,width);
    //     auto Cb_matrix = helper::create_2d_vector<unsigned char>(height/2 ,width/2);
    //     auto Cr_matrix = helper::create_2d_vector<unsigned char>(height/2 ,width/2);

    //     dct::undo_partition_channel(Y_uncompressed, height, width, Y_matrix);
    //     dct::undo_partition_channel(Cb_uncompressed, height/2 ,width/2, Cb_matrix);
    //     dct::undo_partition_channel(Cr_uncompressed, height/2 ,width/2, Cr_matrix);

    //     // Create and fill frame
    //     YUVFrame420 frame {width, height};
    //     for (u32 y = 0; y < height; y++)
    //         for (u32 x = 0; x < width; x++)
    //             frame.Y(x,y) = Y_matrix.at(y).at(x);
    //     for (u32 y = 0; y < height/2; y++)
    //         for (u32 x = 0; x < width/2; x++){
    //             frame.Cb(x,y) = Cb_matrix.at(y).at(x);
    //             frame.Cr(x,y) = Cr_matrix.at(y).at(x);
    //         }
    //     return frame;
    // }

    void compress_I_frame(const std::vector<Block8x8>& Y_blocks, const std::vector<Block8x8>& Cb_blocks, const std::vector<Block8x8>& Cr_blocks,
    std::queue<Block8x8>& Y_uncompressed, std::queue<Block8x8>& Cb_uncompressed, std::queue<Block8x8>& Cr_uncompressed, dct::Quality quality, OutputBitStream& output_stream){

        // Push flag
        output_stream.push_byte(1);

        // I-frames do not use the prev-data
        while(!Y_uncompressed.empty())
            Y_uncompressed.pop();
        while(!Cb_uncompressed.empty())
            Cb_uncompressed.pop();
        while(!Cr_uncompressed.empty())
            Cr_uncompressed.pop();

        // Push active frame data
        for(const Block8x8& curr_block : Y_blocks){
            // Take the DCT and quantize
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(curr_block), quality, dct::luminance);
            // Push in array format
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            // Unquantize and take the inverse DCT
            Y_uncompressed.push(dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, dct::luminance)));
        }
        for(const Block8x8& curr_block : Cb_blocks){
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(curr_block), quality, dct::chrominance);
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            Cb_uncompressed.push(dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, dct::chrominance)));
        }
        for(const Block8x8& curr_block : Cr_blocks){
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(curr_block), quality, dct::chrominance);
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            Cr_uncompressed.push(dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, dct::chrominance)));
        }
    }

    void compress_P_frame(const std::vector<Block8x8>& Y_blocks, const std::vector<Block8x8>& Cb_blocks, const std::vector<Block8x8>& Cr_blocks,
    std::queue<Block8x8>& Y_uncompressed, std::queue<Block8x8>& Cb_uncompressed, std::queue<Block8x8>& Cr_uncompressed, dct::Quality quality, OutputBitStream& output_stream){

        // Push flag
        output_stream.push_byte(2);

        // Push active frame data
        for(const Block8x8& curr_block : Y_blocks){
            // Get previous block
            Block8x8& prev_block = Y_uncompressed.front();
            // Calculate block of delta values
            Block8x8 delta_block = dct::get_delta_block(curr_block, prev_block);
            // Take the DCT and quantize the delta values
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(delta_block), quality, dct::luminance);
            // Push in array format the delta values
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            // Unquantize and take the inverse DCT of the delta values 
            Block8x8 uncompressed_delta = dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, dct::luminance));
            // Remove the used prev_block
            Y_uncompressed.pop();
            // Add uncompressed delta values to the prev_block
            Y_uncompressed.push(dct::add_delta_block(prev_block, uncompressed_delta));
    
        }
        for(const Block8x8& curr_block : Cb_blocks){
            Block8x8& prev_block = Cb_uncompressed.front();
            Block8x8 delta_block = dct::get_delta_block(curr_block, prev_block);
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(delta_block), quality, dct::chrominance);
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            Block8x8 uncompressed_delta = dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, dct::chrominance));
            Cb_uncompressed.pop();
            Cb_uncompressed.push(dct::add_delta_block(prev_block, uncompressed_delta));
        }
        for(const Block8x8& curr_block : Cr_blocks){
            Block8x8& prev_block = Cr_uncompressed.front();
            Block8x8 delta_block = dct::get_delta_block(curr_block, prev_block);
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(delta_block), quality, dct::chrominance);
            stream::push_quantized_array_delta(output_stream, dct::block_to_array(quantized_block));
            Block8x8 uncompressed_delta = dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, dct::chrominance));
            Cr_uncompressed.pop();
            Cr_uncompressed.push(dct::add_delta_block(prev_block, uncompressed_delta));
        }
    }

    /* -----Decompressor code ----- */
    void decompress_I_frame(u16 width, u16 height, std::vector<Block8x8>& Y_blocks, std::vector<Block8x8>& Cb_blocks, std::vector<Block8x8>& Cr_blocks,
    std::queue<Block8x8>& Y_uncompressed, std::queue<Block8x8>& Cb_uncompressed, std::queue<Block8x8>& Cr_uncompressed, dct::Quality quality, InputBitStream& input_stream){
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

        // I-frames do not use the prev-data
        while(!Y_uncompressed.empty())
            Y_uncompressed.pop();
        while(!Cb_uncompressed.empty())
            Cb_uncompressed.pop();
        while(!Cr_uncompressed.empty())
            Cr_uncompressed.pop();

        // Push active frame data
        for(u16 blocks_read = 0; blocks_read < num_Y_blocks; blocks_read++){
            // Create a block
            Block8x8 block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            // Unquantize and take the inverse dct
            block = dct::get_inverse_dct(dct::unquantize_block(block, quality, dct::luminance));
            // Push to active frame
            Y_blocks.push_back(block);
            // Save for later
            Y_uncompressed.push(block);
        }
        for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
            Block8x8 block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            block = dct::get_inverse_dct(dct::unquantize_block(block, quality, dct::chrominance));
            Cb_blocks.push_back(block);
            Cb_uncompressed.push(block);
        }
        for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
            Block8x8 block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            block = dct::get_inverse_dct(dct::unquantize_block(block, quality, dct::chrominance));
            Cr_blocks.push_back(block);
            Cr_uncompressed.push(block);
        }
    }
}
