#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include "output_stream.hpp"
#include "TypeOneEncoder.hpp"

// To compute CRC32 values, we can use this library
// from https://github.com/d-bahr/CRCpp
#define CRCPP_USE_CPP11
#include "CRC.h"

/* ----- Helper Functions ----- */
void pushFileHeader(OutputBitStream& stream){
    stream.push_bytes( 0x1f, 0x8b, //Magic Number
    0x08, //Compression (0x08 = DEFLATE)
    0x00, //Flags
    0x00, 0x00, 0x00, 0x00, //MTIME (little endian)
    0x00, //Extra flags
    0x03 //OS (Linux)
    );
}

void pushFileFooter(OutputBitStream& stream, u32 crc, u32 bytes_read){
    stream.flush_to_byte();
    stream.push_u32(crc);
    stream.push_u32(bytes_read);
}

void encodeTypeOne(OutputBitStream& stream, u32 block_size, std::array <u8, buffer_size>& block_contents, bool is_last){
    TypeOneEncoder encoder {};
    encoder.Encode(stream, block_size, block_contents, is_last);
}

int main(){

    auto crc_table = CRC::CRC_32().MakeTable();
    u32 crc {};

    OutputBitStream stream {std::cout};
    pushFileHeader(stream);

    std::array< u8, buffer_size > block_contents {};
    u32 block_size {0};
    u32 bytes_read {0};
    char next_byte {};


    if (!std::cin.get(next_byte)){
        //Empty input?
    }else{
        ++bytes_read;
        crc = CRC::Calculate(&next_byte, 1, crc_table); 
        while(1){
            block_contents.at(block_size++) = next_byte;
            if (!std::cin.get(next_byte))
                break;

            ++bytes_read;
            crc = CRC::Calculate(&next_byte,1, crc_table, crc); 
            
            if (block_size == block_contents.size()){
                encodeTypeOne(stream, block_size, block_contents, 0);
                block_size = 0;
            }
        }
    }
    
    if (block_size > 0){
        encodeTypeOne(stream, block_size, block_contents, 1);        
    }

    pushFileFooter(stream, crc, bytes_read);
    
    return 0;
}