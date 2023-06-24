#include "prefix_codes.hpp"
#include <cassert>
#include <iostream>

#define assertm(exp, msg) assert(((void)msg, exp))

void check_ll_codes(){
    LLCodesBlock_1 ll_codes {};
    assert(ll_codes.getCodeSequence(0) == (std::vector<bool>{0,0,1,1,0,0,0,0}));
    assert(ll_codes.getCodeSequence(1) == (std::vector<bool>{0,0,1,1,0,0,0,1}));

    assert(ll_codes.getCodeSequence(144) == (std::vector<bool>{1,1,0,0,1,0,0,0,0}));
    assert(ll_codes.getCodeSequence(145) == (std::vector<bool>{1,1,0,0,1,0,0,0,1}));

    assert(ll_codes.getCodeSequence(256) == (std::vector<bool>{0,0,0,0,0,0,0}));

    // check getDistanceSymbol
    assert(ll_codes.getLengthSymbol(3) == 257);
    assert(ll_codes.getLengthSymbol(12) == 265);
    assert(ll_codes.getLengthSymbol(13) == 266);
    assert(ll_codes.getLengthSymbol(34) == 272);
    assert(ll_codes.getLengthSymbol(35) == 273);
    assert(ll_codes.getLengthSymbol(258) == 285);

    // check getLegnthOffset
    assert(ll_codes.getLengthOffset(257) == 0);
    assert(ll_codes.getLengthOffset(265) == 1);
    assert(ll_codes.getLengthOffset(272) == 2);
    assert(ll_codes.getLengthOffset(273) == 3);
    assert(ll_codes.getLengthOffset(285) == 0);

    // check getBaseLength
    assert(ll_codes.getBaseLength(3) == 3);
    assert(ll_codes.getBaseLength(12) == 11);
    assert(ll_codes.getBaseLength(28) == 27);
    assert(ll_codes.getBaseLength(200) == 195);
    assert(ll_codes.getBaseLength(258) == 258);
}

void check_distance(){
    DistanceCodesBlock_1 d_codes{};

    // check getDistanceSymbol
    std::cout << (d_codes.getDistanceSymbol(3) == 2) << "\n";
    assert(d_codes.getDistanceSymbol(193) == 15);
    assert(d_codes.getDistanceSymbol(256) == 15);
    assert(d_codes.getDistanceSymbol(257) == 16);
    assert(d_codes.getDistanceSymbol(384) == 16);
    assert(d_codes.getDistanceSymbol(385) == 17);

    // check getNumOffset
    assert(d_codes.getNumOffset(2) == 0);
    assert(d_codes.getNumOffset(3) == 0);
    assert(d_codes.getNumOffset(7) == 2);
    assert(d_codes.getNumOffset(8) == 3);
    assert(d_codes.getNumOffset(28) == 13);
    assert(d_codes.getNumOffset(29) == 13);

    assert(d_codes.getCodeSequence(3) == (std::vector<bool>{0,0,0,1,1}));

    // check getBasedDistance
    assert(d_codes.getBaseDistance(2) == 2);
    assert(d_codes.getBaseDistance(15) == 13);
    assert(d_codes.getBaseDistance(200) == 193);
    assert(d_codes.getBaseDistance(3000) == 2049);
}

void check_error(){
    DistanceCodesBlock_1 d_codes{};

    std::cerr << "get symbol for 21: " << d_codes.getDistanceSymbol(21) << "\n";
    std::cerr << "get base for 21: " << d_codes.getBaseDistance(21) << "\n";
    std::cerr << "get num offset 21: " << d_codes.getNumOffset(8) << "\n";
    
}

void check_package_merge(){
    std::vector < std::pair <std::vector<u32>, double> > input {};

    input.push_back({{1}, 0.11});
    input.push_back({{2}, 0.15});
    input.push_back({{3}, 0.45});
    input.push_back({{4}, 0.08});
    input.push_back({{5}, 0.01});
    input.push_back({{6}, 0.20});

    PackageMerge pm {};
    pm.getSymbolLengths(input, 6);

}

int main(){
    //check_ll_codes();

    //check_distance();

    //check_error();
    
    check_package_merge();

    return 0;
}