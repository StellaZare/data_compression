#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>


using u32 = std::uint32_t;

static bool sortByProb(const std::pair <std::vector<int>, double> &a, const std::pair <std::vector<int>, double> &b){
    return (a.second < b.second);
}

template<typename T>
void appendVectors(std::vector<T>& dest, const std::vector<T>& src1, const std::vector<T>& src2) {
    dest.insert(dest.end(), src1.begin(), src1.end());
    dest.insert(dest.end(), src2.begin(), src2.end());
}

int countOccurances(std::vector < std::pair <std::vector<int>, double> >& probabilities, u32 symbol){
    u32 count {0};
    for (const auto& pair : probabilities){         // traverse properties vector
        for (const auto& curr_symbol : pair.first) {     // traverse set of symbol
            if (curr_symbol == symbol){
                ++count;
            }
        }
    }
    return count;
}

std::vector< int > construct_canonical_code( std::vector<int> const & lengths ){
    unsigned int size = lengths.size();

    std::vector< unsigned int > length_counts(16,0); //Lengths must be less than 16 for DEFLATE
    int max_length = 0;
    for(auto i: lengths){
        assert(i <= 15);
        length_counts.at(i)++;
        max_length = std::max(i, max_length);
    }
    length_counts[0] = 0; //Disregard any codes with alleged zero length

    std::vector< int > result_codes(size,0);

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

std::vector<int> getSymbolLengths(const std::vector < std::pair <std::vector<int>, double> > probabilities, u32 num_symbols) {
    u32 prob_size = probabilities.size();

    std::vector < std::pair <std::vector<int>, double> > current {probabilities};
    while(current.size() < (2*prob_size)-2){
        // sort with increasing probabilities
        sort(current.begin(), current.end(), sortByProb);
        // std::cerr << "---- sort ----" << std::endl;
        // printPairVector(current);

        // if odd number of packages -> discard last (if it not one of the original packages)
        if(current.size()%2 != 0){
            current.pop_back();
        }
        std::vector < std::pair <std::vector<int>, double> > packages {};
        for(int idx = 0; idx < current.size()-1; idx += 2){
            // create package
            std::vector<int> v {};
            appendVectors(v, current.at(idx).first, current.at(idx+1).first);
            double prob = current.at(idx).second + current.at(idx+1).second;
            // merge
            packages.push_back({v, prob});
        }
        // std::cerr << "---- packages ----" << std::endl;
        // printPairVector(packages);

        current.clear();
        appendVectors(current, probabilities, packages);
        
        // std::cerr << "---- done ----" << std::endl;
        // printPairVector(current);
    }
    

    std::vector<int> lengths_table {};
    for(int symbol = 0; symbol < num_symbols; ++symbol){
        lengths_table.push_back(countOccurances(current, symbol));
    }

    //printLengths(lengths_table);

    return lengths_table;
}

int main(){
    // -100 = x; 100 = x; 0000 = 110; 0000 0000 = 120; 150 = Omega;
    std::vector<int> symbols {-100, -5, -4, -3, -2, -1 , 0, 1, 2, 3, 4, 5, 100, 110, 120, 150};

    // Initialize a vector of pairs using explicit constructor calls
    std::vector<std::pair<std::vector<int>, double>> probabilities;

    // Create pairs and push them into the vector
    probabilities.push_back(std::make_pair(std::vector<int>{-100},  7731));
    probabilities.push_back(std::make_pair(std::vector<int>{-5},    3456));
    probabilities.push_back(std::make_pair(std::vector<int>{-4},    7083));
    probabilities.push_back(std::make_pair(std::vector<int>{-3},    18250));
    probabilities.push_back(std::make_pair(std::vector<int>{-2},    55600));
    probabilities.push_back(std::make_pair(std::vector<int>{-1},    382876));
    probabilities.push_back(std::make_pair(std::vector<int>{0},     44659806));
    probabilities.push_back(std::make_pair(std::vector<int>{1},     375799));
    probabilities.push_back(std::make_pair(std::vector<int>{2},     58418));
    probabilities.push_back(std::make_pair(std::vector<int>{3},     20113));
    probabilities.push_back(std::make_pair(std::vector<int>{4},     7785));
    probabilities.push_back(std::make_pair(std::vector<int>{5},     3571));
    probabilities.push_back(std::make_pair(std::vector<int>{100},   18712));
    probabilities.push_back(std::make_pair(std::vector<int>{110},   50432));
    probabilities.push_back(std::make_pair(std::vector<int>{120},   80432));
    probabilities.push_back(std::make_pair(std::vector<int>{150},   71280));



    double sum = 0;

    for(u32 idx = 0; idx < 16; idx++){
        sum += probabilities.at(idx).second;
    }
    for(u32 idx = 0; idx < 16; idx++){
        probabilities.at(idx).second /= sum;
    }
    for(u32 idx = 0; idx < 16; idx++){
        std::cerr << probabilities.at(idx).first.at(0) << " prob " << probabilities.at(idx).second << std::endl;
    }

    std::vector<int> code_lengths = getSymbolLengths(probabilities, 16);
    for(u32 idx = 0; idx < 16; idx++){
        if(code_lengths.at(idx) == 0)
            code_lengths.at(idx) = 1;
    }

    std::vector<int> encoding = construct_canonical_code(code_lengths);

    for(u32 idx = 0; idx < 16; idx++){
        std::cerr << symbols.at(idx) << " length " << code_lengths.at(idx) << " enconding " << encoding.at(idx) << std::endl;
    }

    return 0;
}