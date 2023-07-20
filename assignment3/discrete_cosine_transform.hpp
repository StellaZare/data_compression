#include <vector>
#include <cmath>
#include <array>

using Block8x8 = std::array<std::array<double, 8>, 8>;
using u32 = std::uint32_t;

namespace dct{

    // the result of running create_c_matrix() from discrete_cosine_transfrom.cpp  
    const Block8x8 c_matrix {{
        {0.353553,  0.353553,   0.353553,   0.353553,   0.353553,   0.353553,   0.353553,   0.353553    },
        {0.490393,  0.415735,   0.277785,   0.0975452,  -0.0975452, -0.277785,  -0.415735,  -0.490393   },
        {0.46194,   0.191342,   -0.191342,  -0.46194,   -0.46194,   -0.191342,  0.191342,   0.46194     },
        {0.415735,  -0.0975452, -0.490393,  -0.277785,  0.277785,   0.490393,   0.0975452,  -0.415735   },
        {0.353553,  -0.353553,  -0.353553,  0.353553,   0.353553,   -0.353553,  -0.353553,  0.353553    },
        {0.277785,  -0.490393,  0.0975452,  0.415735,   -0.415735,  -0.0975452, 0.490393,   -0.277785   },
        {0.191342,  -0.46194,   0.46194,    -0.191342,  -0.191342,  0.46194,    -0.46194,   0.191342    },
        {0.0975452, -0.277785,  0.415735,   -0.490393,  0.490393,   -0.415735,  0.277785,   -0.0975452  }
    }};
    // the result of running transpose_block() from discrete_cosine_transfrom.cpp 
    const Block8x8 c_matrix_transpose {{
        {0.353553, 0.490393,   0.46194,   0.415735,   0.353553,  0.277785,   0.191342,  0.0975452   },
        {0.353553, 0.415735,   0.191342,  -0.0975452, -0.353553, -0.490393,  -0.46194,  -0.277785   },
        {0.353553, 0.277785,   -0.191342, -0.490393,  -0.353553, 0.0975452,  0.46194,   0.415735    },
        {0.353553, 0.0975452,  -0.46194,  -0.277785,  0.353553,  0.415735,   -0.191342, -0.490393   },
        {0.353553, -0.0975452, -0.46194,  0.277785,   0.353553,  -0.415735,  -0.191342, 0.490393    },
        {0.353553, -0.277785,  -0.191342, 0.490393,   -0.353553, -0.0975452, 0.46194,   -0.415735   },
        {0.353553, -0.415735,  0.191342,  0.0975452,  -0.353553, 0.490393,   -0.46194,  0.277785    },
        {0.353553, -0.490393,  0.46194,   -0.415735,  0.353553,  -0.277785,  0.191342,  -0.0975452  }
    }};

    // quantization matrix used by JPEG - from lecture slides
    const Block8x8 luminance {{
        {16, 11, 10, 16, 24,  40,  51,  61},
        {12, 12, 14, 19, 26,  58,  60,  55},
        {14, 13, 16, 24, 40,  57,  69,  56},
        {14, 17, 22, 29, 51,  87,  80,  62},
        {18, 22, 37, 56, 68,  109, 103, 77},
        {24, 35, 55, 64, 81,  104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99}
    }};

    // quantization matrix used by JPEG - from lecture slides
    const Block8x8 chrominance {{
        {17, 18, 24, 47, 99, 99, 99, 99},
        {18, 21, 26, 66, 99, 99, 99, 99},
        {24, 26, 56, 99, 99, 99, 99, 99},
        {47, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
    }};

    // prints 8x8 block to standard out
    void print_block8x8(const Block8x8& matrix){
        for(u32 r = 0; r < 8; r++){
            std::cout << "{ ";
            for(u32 c = 0; c < 8; c++){
                std::cout << matrix.at(r).at(c) << " ";
            }
            std::cout << "}" << std::endl;
        }
    }

    // given a color channel partitions into 8x8 blocks and adds blocks to vector in row major order
    void partition_channel(std::vector<Block8x8>& blocks, u32 height, u32 width, std::vector<std::vector<unsigned char>> channel){
        for(u32 r = 0; r < height; r+=8){
            for(u32 c = 0; c < width; c+=8){
                Block8x8 current_block;
                // index into 8x8 sub block
                for(u32 sub_r = 0; sub_r < 8; sub_r++){
                    for(u32 sub_c = 0; sub_c < 8; sub_c++){
                        // copy element 
                        if((r+sub_r) >= height && (c+sub_c) < width){
                            current_block.at(sub_r).at(sub_c) = double(channel.at(height-1).at(c+sub_c));
                        }else if((c+sub_c) >= width && (r+sub_r) < height){
                            current_block.at(sub_r).at(sub_c) = double(channel.at(r+sub_r).at(width-1));
                        }else if((r+sub_r) >= height && (c+sub_c) >= width){
                            current_block.at(sub_r).at(sub_c) = double(channel.at(height-1).at(width-1));
                        }else{
                            current_block.at(sub_r).at(sub_c) = double(channel.at(r+sub_r).at(c+sub_c));
                        }
                    }
                }
                blocks.push_back(current_block);
            }
        }
    }

}