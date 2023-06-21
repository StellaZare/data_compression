#include "output_stream.hpp"
#include "LZSSEncoder.hpp"
#include "prefix_codes.hpp"

// To compute CRC32 values, we can use this library
// from https://github.com/d-bahr/CRCpp
#define CRCPP_USE_CPP11
#include "CRC.h"

const u32 buffer_size = (1<<16)-1;

class TypeOneEncoder {
    public:
    TypeOneEncoder (){}

    /*
        Encodes block type 1 by calling the LZSSEncoder_2 class on the block contents
        then decomposes the Code vector returned and pushes bits to the stream
    */
    void Encode(OutputBitStream& stream, u32 block_size, std::array <u8, buffer_size>& block_contents, bool is_last){
        pushBlockHeader(stream, is_last);

        LZSSEncoder_2 lzss {};

        const std::vector<Code>& result = lzss.Encode(block_size, block_contents);

        for(const Code& curr : result){
            if (curr.code_type == 1){
                pushLiteral(stream, curr.value);
            }
            else if (curr.code_type == 2){
                pushLength(stream, curr.value);
            }
            else if (curr.code_type == 3){
                pushDistance(stream, curr.value);
            }
            else{
                std::cerr << "invalid code type in result" << std::endl;
            }
        }

        pushEOB(stream);
    }

    private:
    LLCodesBlock_1 ll_codes {};
    DistanceCodesBlock_1 distance_codes {};

    void pushBlockHeader(OutputBitStream& stream, bool& is_last){
        stream.push_bit(is_last); 
        stream.push_bits(1, 2); //Two bit block type 
    }

    void pushLiteral(OutputBitStream& stream, u32 literal){
        std::vector<bool> seq = ll_codes.getCodeSequence(literal);
        for(u32 bit : seq){
            stream.push_bit(bit);
        }
    }

    void pushLength(OutputBitStream& stream, u32 length){
        u32 length_symbol = ll_codes.getLengthSymbol(length);
        u32 base_length = ll_codes.getBaseLength(length);
        u32 length_offset = ll_codes.getLengthOffset(length_symbol);

        for(u8 bit : ll_codes.getCodeSequence(length_symbol)){
            stream.push_bit(bit);
        }
        stream.push_bits(length - base_length, length_offset);       
    }

    void pushDistance(OutputBitStream& stream, u32 distance){
        u32 distance_symbol = distance_codes.getDistanceSymbol(distance);
        u32 base_distance = distance_codes.getBaseDistance(distance);
        u32 distance_offset = distance_codes.getNumOffset(distance_symbol);

        for(u8 bit : distance_codes.getCodeSequence(distance_symbol)){
            stream.push_bit(bit);
        }
        stream.push_bits(distance - base_distance, distance_offset);   
    }

    void pushEOB(OutputBitStream& stream){
        std::vector<bool> EOB = ll_codes.getCodeSequence(256);
        for(auto bit : EOB){
            stream.push_bit(bit);
        }
    }

};