#ifndef DISCRETE_COSINE_TRANSFORM
#define DISCRETE_COSINE_TRANSFORM

#include <vector>
#include <cmath>
#include <array>

using Block8x8 = std::array<std::array<double, 8>, 8>;
using Array64 = std::array<int, 64>;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

namespace dct {

    enum Quality {
        low = 0,
        medium,
        high,
        ERROR
    };

    enum Direction {
        right = 0,
        down,
        down_left,
        up_right
    };

    /* ----- Written by Bill ------ */
    inline unsigned char round_and_clamp_to_char(double v){
        //Round to int 
        int i = (int)(v+0.5);
        //Clamp to the range [0,255]
        if (i < 0)
            return 0;
        else if (i > 255)
            return 255;
        return i;
    }

    /* ----- Block Operations ----- */
    const Block8x8 create_c_matrix();
    void print_array(const Array64 &array);
    void print_block(const Block8x8 &block);
    void print_blocks(const std::vector<Block8x8> &blocks);
    Block8x8 multiply_block(const Block8x8 &blockA, const Block8x8 &blockB);
    Block8x8 transpose_block(const Block8x8 &block);

    /* ----- Compressor Functions ----- */
    void partition_channel(std::vector<Block8x8> &blocks, u32 height, u32 width, const std::vector<std::vector<unsigned char>> &channel);
    Block8x8 get_dct(const Block8x8 &block);
    Block8x8 quantize_block(const Block8x8 &block, Quality quality, const Block8x8 &q_matrix);
    Direction get_direction(u32 r, u32 c, Direction curr);
    Array64 block_to_array(const Block8x8 &block);

    /* ----- Decompressor Functions ----- */
    Block8x8 array_to_block(const Array64 &array);
    Block8x8 unquantize_block(const Block8x8 &block, Quality quality, const Block8x8 &q_matrix);
    Block8x8 get_inverse_dct(const Block8x8 &block);
    void undo_partition_channel(const std::vector<Block8x8> &blocks, u32 height, u32 width, std::vector<std::vector<unsigned char>> &channel);

} // namespace dct

#endif
