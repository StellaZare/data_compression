#include <queue>
#include <vector>
#include <string>
#include <list>
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

    /*----- Deprocated ------*/
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

    void decompress_I_frame(u16 num_Y_blocks, u16 num_C_blocks, std::vector<Block8x8>& Y_blocks, std::vector<Block8x8>& Cb_blocks, std::vector<Block8x8>& Cr_blocks,
    std::queue<Block8x8>& Y_uncompressed, std::queue<Block8x8>& Cb_uncompressed, std::queue<Block8x8>& Cr_uncompressed, dct::Quality quality, InputBitStream& input_stream){

        // I-frames do not use the prev-data
        while(!Y_uncompressed.empty())
            Y_uncompressed.pop();
        while(!Cb_uncompressed.empty())
            Cb_uncompressed.pop();
        while(!Cr_uncompressed.empty())
            Cr_uncompressed.pop();

        // Push active frame data
        for(u16 blocks_read = 0; blocks_read < num_Y_blocks; blocks_read++){
            // Read a block
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

    void decompress_P_frame(u16 num_Y_blocks, u16 num_C_blocks, std::vector<Block8x8>& Y_blocks, std::vector<Block8x8>& Cb_blocks, std::vector<Block8x8>& Cr_blocks,
    std::queue<Block8x8>& Y_uncompressed, std::queue<Block8x8>& Cb_uncompressed, std::queue<Block8x8>& Cr_uncompressed, dct::Quality quality, InputBitStream& input_stream){

        // Push active frame data
        for(u16 blocks_read = 0; blocks_read < num_Y_blocks; blocks_read++){
            // Create a block of delta values
            Block8x8 delta_block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            // Unquantize and take the inverse dct
            delta_block = dct::get_inverse_dct(dct::unquantize_block(delta_block, quality, dct::luminance));
            // Get the prev_block
            Block8x8& prev_block = Y_uncompressed.front();
            Block8x8 block = dct::add_delta_block(prev_block, delta_block);
            // Push to active frame
            Y_blocks.push_back(block);
            // Save for later
            Y_uncompressed.push(block);
            // Remove used prev_block
            Y_uncompressed.pop();
        }
        for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
            Block8x8 delta_block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            delta_block = dct::get_inverse_dct(dct::unquantize_block(delta_block, quality, dct::chrominance));
            Block8x8& prev_block = Cb_uncompressed.front();
            Block8x8 block = dct::add_delta_block(prev_block, delta_block);
            // Push to active frame
            Cb_blocks.push_back(block);
            // Save for later
            Cb_uncompressed.push(block);
            // Remove used prev_block
            Cb_uncompressed.pop();
        }
        for(u16 blocks_read = 0; blocks_read < num_C_blocks; blocks_read++){
            Block8x8 delta_block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            delta_block = dct::get_inverse_dct(dct::unquantize_block(delta_block, quality, dct::chrominance));
            Block8x8& prev_block = Cr_uncompressed.front();
            Block8x8 block = dct::add_delta_block(prev_block, delta_block);
            // Push to active frame
            Cr_blocks.push_back(block);
            // Save for later
            Cr_uncompressed.push(block);
            // Remove used prev_block
            Cr_uncompressed.pop();
        }

    }
    /* ----- Compressor Code ----- */

    void compress_I_block(std::list<Block8x8>& compressed_blocks, std::list<Block8x8>& uncompressed_blocks, u32 C_idx, 
    const std::vector<Block8x8>& Y_blocks, const std::vector<Block8x8>& Cb_blocks, const std::vector<Block8x8>& Cr_blocks, dct::Quality quality){
        u32 Y_idx = 4 * C_idx;
        for(u32 count = 0; count < 4; count++){
            // Take the DCT and quantize
            Block8x8 quantized_block = dct::quantize_block(dct::get_dct(Y_blocks.at(Y_idx+count)), quality, dct::luminance);
            // Push in array format
            compressed_blocks.push_back(quantized_block);
            // Unquantize and take the inverse DCT
            uncompressed_blocks.push_back(dct::get_inverse_dct(dct::unquantize_block(quantized_block, quality, dct::luminance)));
        }

        Block8x8 quantized_Cb_block = dct::quantize_block(dct::get_dct(Cb_blocks.at(C_idx)), quality, dct::chrominance);
        compressed_blocks.push_back(quantized_Cb_block);
        uncompressed_blocks.push_back(dct::get_inverse_dct(dct::unquantize_block(quantized_Cb_block, quality, dct::chrominance)));

        Block8x8 quantized_Cr_block = dct::quantize_block(dct::get_dct(Cr_blocks.at(C_idx)), quality, dct::chrominance);
        compressed_blocks.push_back(quantized_Cr_block);
        uncompressed_blocks.push_back(dct::get_inverse_dct(dct::unquantize_block(quantized_Cr_block, quality, dct::chrominance)));
    }

    void push_compressed_blocks(const std::list<bool>& flags, std::list<Block8x8>& compressed_blocks, OutputBitStream& output_stream){
        for(bool block_type : flags){
            // Push block-type bit (0=I-block and 1=P-block)
            output_stream.push_bit(block_type);
            // Push the macro block (in Y Cb Cr order)
            for(u32 count = 0; count < 6; count++){
                stream::push_quantized_array_delta(output_stream, dct::block_to_array(compressed_blocks.front()));
                compressed_blocks.pop_front();
            }
        }
    }

    YUVFrame420 reconstruct_prev_frame(std::list<Block8x8>& compressed_blocks, u32 num_macro_blocks, u32 height, u32 width){  
        std::vector<Block8x8> Y_blocks, Cb_blocks, Cr_blocks;
        for(u32 macro_count = 0; macro_count < num_macro_blocks; macro_count++){
            for(u32 y_count = 0; y_count < 4; y_count++){
                Y_blocks.push_back(compressed_blocks.front());
                compressed_blocks.pop_front();
            }
            Cb_blocks.push_back(compressed_blocks.front());
            compressed_blocks.pop_front();

            Cr_blocks.push_back(compressed_blocks.front());
            compressed_blocks.pop_front();
        }

        auto Y_matrix = helper::create_2d_vector<unsigned char>(height,width);
        auto Cb_matrix = helper::create_2d_vector<unsigned char>(height/2 ,width/2);
        auto Cr_matrix = helper::create_2d_vector<unsigned char>(height/2 ,width/2);
        dct::undo_partition_Y_channel(Y_blocks, height, width, Y_matrix);
        dct::undo_partition_C_channel(Cb_blocks, height/2 ,width/2, Cb_matrix);
        dct::undo_partition_C_channel(Cr_blocks, height/2 ,width/2, Cr_matrix);

        YUVFrame420 previous_frame {width, height};
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                previous_frame.Y(x,y) = Y_matrix.at(y).at(x);
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++){
                previous_frame.Cb(x,y) = Cb_matrix.at(y).at(x);
                previous_frame.Cr(x,y) = Cr_matrix.at(y).at(x);
            }
        return previous_frame;

    }

    /*----- Decompressor Code -----*/
    void decompress_I_block(std::vector<Block8x8>& Y_blocks, std::vector<Block8x8>& Cb_blocks, std::vector<Block8x8>& Cr_blocks, dct::Quality quality, InputBitStream& input_stream){
        for(u32 count = 0; count < 4; count++){
            Block8x8 Y_block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
            // Unquantize and take the inverse dct
            Y_blocks.push_back(dct::get_inverse_dct(dct::unquantize_block(Y_block, quality, dct::luminance)));
        }
        Block8x8 Cb_block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
        Cb_blocks.push_back(dct::get_inverse_dct(dct::unquantize_block(Cb_block, quality, dct::chrominance)));

        Block8x8 Cr_block = dct::array_to_block(stream::read_quantized_array_delta(input_stream));
        Cr_blocks.push_back(dct::get_inverse_dct(dct::unquantize_block(Cr_block, quality, dct::chrominance)));
    }
}
