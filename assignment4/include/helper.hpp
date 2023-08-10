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

    void process_I_frame(const std::vector<Block8x8>& Y_blocks, const std::vector<Block8x8>& Cb_blocks, const std::vector<Block8x8>& Cr_blocks,
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
}
