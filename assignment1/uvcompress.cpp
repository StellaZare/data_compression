/* uvcompress.cpp
   CSC 485B/CSC 578B

   Placeholder starter code for UVCompress 

   B. Bird - 2023-05-01
*/

#include <iostream>
#include <unordered_map>
#include <string>
#include <bitset>
#include <vector>
#include <cmath>

class SymbolTable {
    public:
        SymbolTable(){
            for(int idx = 0; idx < 256; ++idx){
                table_[std::string(1,char(idx))] = idx;
            }
        }

        void addSymbol(const std::string& symbol){
            table_[symbol] = next_idx_;
            ++next_idx_;
        }

        bool contains(const std::string& symbol) const {
            return table_.find(symbol) != table_.end();
        }

        uint16_t getSymbolIndex(const std::string& symbol) const {
            return table_.at(symbol);
        }

        uint16_t getNextIndex() const {
            return next_idx_;
        }

    private:
        // next index to insert (256 reserved)
        uint16_t next_idx_ {257};
        // dictionary of (key: symbol , value: index)
        std::unordered_map<std::string, int> table_;
};

class BitStream {
    public:
        BitStream(){
            std::cout.put(0x1f);
            std::cout.put(0x9d);
            std::cout.put(0x90);
        }

        void addBit(bool bit){
            stream.push_back(bit);
            ++size_;

            if (size_ == 8){
                flushStream();
            }
        }

        void flushStream(){
            while(size_ < 8){
                stream.push_back(0);
                ++size_;
            }

            std::bitset<8> bitset;
            for (int i = 0; i < 8; ++i) {
                bitset[i] = stream[i];
            }
            //std::cout << "Bitset: " << bitset.to_string() << std::endl;

            unsigned char byte = static_cast<unsigned char>(bitset.to_ulong());
            std::cout.put(byte);
            stream.clear();
            size_ = 0;
        }

    private:
        std::vector<bool> stream;
        int size_ {0};

        
};

int main(){

    SymbolTable symbolTable;
    BitStream stream;

    int num_bits{9};
    const int max_bits{16};

    char c {};
    std::string augmented{""};
    std::string working{""};

    while(std::cin.get(c)){
        std::string current{c};

        augmented = working + current;

        if (symbolTable.contains(augmented)){
            working = augmented;

        }else if (symbolTable.getNextIndex() >= std::pow(2, 16)){
            uint index = symbolTable.getSymbolIndex(working);
            std::bitset<max_bits> binary{index};
            std::string binaryString = binary.to_string();

            // std::cout << "output: " << working << "\n";

            for (int idx = max_bits - num_bits; idx < max_bits; ++idx) {
                stream.addBit(binaryString[idx] == '1'); // Convert char '1'/'0' to bool
            }
            working = current;
            
        }else{
            symbolTable.addSymbol(augmented);
            uint index = symbolTable.getSymbolIndex(working);
            std::bitset<max_bits> binary{index};
            std::string binaryString = binary.to_string();

            // std::cout << "output: " << working << "\n";
            for (int idx = max_bits - 1; idx >= max_bits - num_bits; --idx) {
                stream.addBit(binaryString[idx] == '1'); // Convert char '1'/'0' to bool
            }

            working = current;
            if (symbolTable.getNextIndex() >= std::pow(2, num_bits)){
                ++num_bits;
            }
        }
    }

    if(!working.empty()){
        uint index = symbolTable.getSymbolIndex(working);
        std::bitset<max_bits> binary{index};
        std::string binaryString = binary.to_string();

        // std::cout << "output: " << working << "\n";
        for (int idx = max_bits - 1; idx >= max_bits - num_bits; --idx) {
            stream.addBit(binaryString[idx] == '1'); // Convert char '1'/'0' to bool
        }
    }

    stream.flushStream();

    return 0;
}