/* uvcompress.cpp
   CSC 485B/CSC 578B

   Placeholder starter code for UVCompress 

   B. Bird - 2023-05-01
*/

#include <iostream>
#include <unordered_map>
#include <string>

class SymbolTable {
    public:
        SymbolTable(){
            for(int idx = 0; idx < 256; ++idx){
                table_[std::string(1,char(idx))] = idx;
            }
        }

        void addSymbol(const std::string& symbol){
            table_[symbol] = size_;
            ++size_;
        }

        bool contains(const std::string& symbol) const {
            return table_.find(symbol) != table_.end();
        }

        int getIndex(const std::string& symbol) const {
            return table_.at(symbol);
        }

    private:
        // tracks the current size of the table
        int size_ {256};
        // dictionary of (key: symbol , value: index)
        std::unordered_map<std::string, int> table_;
};

int main(){

    //This placeholder code reads bytes from stdin and writes the bitwise complement
    //to stdout (obviously this is useless for the assignment, but maybe it gives some impression
    //of how I/O works in C++)

    char c {};
    //The .get() method of std::istream objects (like the standard input stream std::cin) reads a single unformatted
    //byte into the provided character (notice that c is passed by reference and is modified by the method).
    //If the byte cannot be read, the return value of the method is equivalent to the boolean value 'false'.
    // while(std::cin.get(c)){

    //     //The .put() method of std::ostream objects (like std::cout) writes a single unformatted byte.
    //     char c_complement {};
    //     c_complement = ~c;
    //     std::cout.put(c_complement);
    // }`

    return 0;
}