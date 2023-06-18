/* uvgz.cpp

   Starter code for Assignment 2 (in C++).
   This basic implementation generates a fully compliant .gz output stream,
   using block mode 0 (store only) for all DEFLATE data.

   B. Bird - 2023-05-12
*/
#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include "output_stream.hpp"
#include "LZSS.hpp"

// To compute CRC32 values, we can use this library
// from https://github.com/d-bahr/CRCpp
#define CRCPP_USE_CPP11
#include "CRC.h"

const u32 buffer_size = (1<<16)-1;

void pushBlockType_0(OutputBitStream& stream, u32 block_size, std::array <u8, buffer_size >& block_contents, u8 last_bit){
    //We know that there are more bytes left, so this is not the last block
    stream.push_bit(last_bit); 
    stream.push_bits(0, 2); //Two bit block type 0
    stream.push_bits(0, 5); //Pad the stream

    stream.push_u16(block_size);
    stream.push_u16(~block_size);

    //Now write the actual block data
    for(unsigned int i = 0; i < block_size; i++)
        stream.push_byte(block_contents.at(i)); //Interesting optimization question: Will the compiler optimize away the bounds checking for .at here?
}

void pushBlockType_1(LZSSEncoder& lzss_encoder , OutputBitStream& stream, u32 block_size, std::array <u8, (1<<16)-1>& block_contents, u8 last_bit){  
    lzss_encoder.encodeBlock(block_contents, block_size);
}

int main(){

    OutputBitStream stream {std::cout};
    auto crc_table = CRC::CRC_32().MakeTable();
    LZSSEncoder lzss_encoder {stream};

    //Push a basic gzip header
    stream.push_bytes( 0x1f, 0x8b, //Magic Number
        0x08, //Compression (0x08 = DEFLATE)
        0x00, //Flags
        0x00, 0x00, 0x00, 0x00, //MTIME (little endian)
        0x00, //Extra flags
        0x03 //OS (Linux)
    );

    std::cerr << "pushed header bytes\n";

    //Note that the types u8, u16 and u32 are defined in the output_stream.hpp header
    const u32 buffer_size = (1<<16)-1;
    std::array< u8, buffer_size > block_contents {};
    u32 block_size {0};
    u32 bytes_read {0};

    char next_byte {}; //Note that we have to use a (signed) char here for compatibility with istream::get()

    //Keep a running CRC of the data we read.
    u32 crc {};

    if (!std::cin.get(next_byte)){
        //Empty input?
        
    }else{

        ++bytes_read;

        crc = CRC::Calculate(&next_byte, 1, crc_table); 

        //Read through the input
        while(1){
            block_contents.at(block_size++) = next_byte;
            if (!std::cin.get(next_byte))
                break;

            ++bytes_read;
            crc = CRC::Calculate(&next_byte,1, crc_table, crc); //Add the character we just read to the CRC (even though it is not in a block yet)

            //If we get to this point, we just added a byte to the block AND there is at least one more byte in the input waiting to be written.
            if (block_size == block_contents.size()){
                std::cerr << "pushing full block\n";

                //pushBlockType_0(stream, block_size, block_contents, 0);
                pushBlockType_1(lzss_encoder, stream, block_size, block_contents, 1);
                block_size = 0;
            }
        }
    }
    //At this point, we've finished reading the input (no new characters remain), and we may have an incomplete block to write.
    if (block_size > 0){
        std::cerr << "pushing incomplete block\n";

        //pushBlockType_0(stream, block_size, block_contents, 1);
        pushBlockType_1(lzss_encoder, stream, block_size, block_contents, 1);
        stream.flush_to_byte();
    }

    std::cerr << "pushing end of block\n";
    lzss_encoder.pushEOB();

    //Now close out the bitstream by writing the CRC and the total number of bytes stored.
    stream.push_u32(crc);
    stream.push_u32(bytes_read);

    return 0;
}