#include <vector>
#include <vector>
#include <iostream>

using u32 = std::uint32_t;

std::vector<u32> construct_canonical_code( std::vector<u32> const& lengths ){

        unsigned int size = lengths.size();
        std::vector< unsigned int > length_counts(16,0); //Lengths must be less than 16 for DEFLATE
        u32 max_length = 0;
        for(auto i: lengths){
            assert(i <= 15);
            length_counts.at(i)++;
            max_length = std::max(i, max_length);
        }
        length_counts[0] = 0; //Disregard any codes with alleged zero length

        std::vector< u32 > result_codes(size,0);

        //The algorithm below follows the pseudocode in RFC 1951
        std::vector< unsigned int > next_code(size,0);
        {
            //Step 1: Determine the first code for each length
            unsigned int code = 0;
            for(unsigned int i = 1; i <= max_length; i++){
                code = (code+length_counts.at(i-1))<<1;
                next_code.at(i) = code;
            }
        }
        {
            //Step 2: Assign the code for each symbol, with codes of the same length being
            //        consecutive and ordered lexicographically by the symbol to which they are assigned.
            for(unsigned int symbol = 0; symbol < size; symbol++){
                unsigned int length = lengths.at(symbol);
                if (length > 0)
                    result_codes.at(symbol) = next_code.at(length)++;
            }  
        } 
        return result_codes;
    }


int main()
{
 
    std::vector<int> symbols { -100, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 100, 120, 150};
    double probability[] = { 
        0.00528108192,
        0.002621367365,
        0.00506663023,
        0.0126354782,
        0.04378956096,
        0.2619029835,
        0.2966863319,
        0.258156177,
        0.04418603243,
        0.01262818411,
        0.005256055659,
        0.002723026115,
        0.005238404228,
        0.04382868637,
        0.02464491912,
        0.3625013234
    };

    std::vector<u32> lengths {9, 9, 8, 7, 5, 2, 2, 3, 6, 7, 8, 9, 9, 5, 2};
 
    // HuffmanCodes(arr, probability, size);
    std::vector<u32> encoding = construct_canonical_code(lengths);

    for(u32 idx = 0; idx < symbols.size(); idx++){
        std::cerr << symbols.at(idx) << "\t" << lengths.at(idx) << "\t" << encoding.at(idx) << std::endl;
    }
 
    return 0;
}
