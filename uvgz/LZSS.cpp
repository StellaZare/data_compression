#include <iostream>
#include <vector>
#include <array>
#include <string>
#include "output_stream.hpp"

#define CRCPP_USE_CPP11
#include "CRC.h"

class LZSSEncoder{
    public:

    LZSSEncoder() = default;

    private:
    std::array <u8, 500> buff {};
    std::array <u8, 256> last_seen{};
    std::size_t start_history {0};
    std::size_t end_lookahead {0};
    std::size_t curr_symbol {0};
};