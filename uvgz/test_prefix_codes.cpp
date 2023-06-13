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
}

int main(){
    check_ll_codes();
    return 0;
}