# Implemented Features
* Block type 1
* Block type 2
* Backreferenced for window size greater than 2048
* Can compress entire collection in 240 secs
* Uses Huffman coding (Package Merge and Cononical Algorithm) to generate dynamic codes
* Optimizes block 2 header 
* Prevents sending back references with a large distance value

# Architecture
## uvgz.cpp
This file is essentially the starter code provided. The main function first pushed the file header bits to the stream by calling _pushFileHeader_; then reads bytes from std::cin and stores the data into an array called _block contents_. When input ends or block contents has reached capacity the block is encoded.

The function _encodeTypeOne_ encodes the chunk of data using Block Type 1 by initializing a _TypeOneEncoder_ object and calling the _Encode_ member function.
Similarly, the function _encodeTypeTwo_ encodes the chunk of data using Block Type 2 by initializing a _TypeTwoEncoder_ object and calling the _Encode_ member function.

Finally, after encoding the chunk of data, the call to _pushFileFooter_ flushes to the next byte boundary then pushes the crc and bytes read value to the stream.

## TypeOneEncoder.hpp
This file contains the _TypeOneEncoder_ class definition. This class receives one chunk of data from main to encode using LZSS.

The _Encode_ member function first pushes the block header bits to the stream; then initialized an _LZSSEncoder 2_ object, calling its _Encode_ member function. This function returns a vector of _Code_ structs which is then processes by the _Encode_ function.

### Code Structure
The _Code_ struct definition is as follows:
```c++
struct Code {
    u8 code_type {0};   // 1:literal 2:length 3:distance 0:default
    u32 value {0};      // 1: 0-255  2:3-258  3:1-32768  0:0
};
```

The _Encode_ function traverses the array of _Code_ objects and processes each based on its _code type_ and pushes the result to the stream. The  _TypeOneEncoder_ uses private [data members](#LLCodesBlock_1-and-DistanceCodesBlock_1) to do this in member functions: _pushLiteral_, _pushLength_ and _pushDistance_.

Lastly, the _Encode_ function pushed the EOB marker to the stream.

### LLCodesBlock_1 and DistanceCodesBlock_1
These classes in _prefix codes.hpp_ are used to maintain the LL and distance tables, and query the encoding of LL and distance symbols with fixed codes. These classes have a very similar interface.

## TypeTwoEncoder.hpp
This file contains the _TypeTwoEncoder_ class definition. This class first encodes the data block with the _LZSSEncoder 2_ class as in block 1.

Then the LL and Distance probabilities are calculated using the LZSS encoding blocks. This is done by first counting the number of occurrences of each symbol, stored in data members LL_count and distance_count. Then dividing by the total count.

The probabilities and are then stored into a vector of pairs where pair.first is a vector containing the symbol and pair.second is the probability of that symbol. There is one probability pair for each LL and distance symbol that appears in the data. 

The probabilities vector is then passed as a parameter into the _getSymbolLengths_ member function of the [_PackageMerge_ class](#PackageMerge). This function returns a vector with a length value for each symbol in the set.

This vector is then passed used to initialize a [_DynamicCodes_](#dynamiccodes) object to assign and retrieve custom encodings.

Then the function _getLLDistanceEncoding_ is called. This function takes as input the LL and Distance code length tables; and combines them to create a vector called _ll and distance_. 
The function traverses this vector to perform RLE on the LL and Distance tables. It uses a helper function _getRunLength_ to get the run length starting at a particular index.
The _getLLDistanceEncoding_ pushes elements into the data members _LL distance encoding_ and _CL count_ as it traverses the code length tables.

#### Hint
_LL distance encoding_ is a vector of the actual table encoding (ie. 18 86 2 2 2 18 127 18 7 2 18 20 1 1 18 17 )
_CL count_ count the number of occurrences for each symbol (ie. 0-18)

Having the CL probabilities the same algorithms are used to generate code lengths, then symbol encodings for the CL table by initializing another _DynamicCodes_ object.

Finally the following function push all the data to the stream:
```c++
pushBlockHeader(stream, is_last, CL_code_lengths);
pushLLDistanceLengths(stream, CL_codes);
pushBitStream(stream, bit_stream, LL_codes, distance_codes );
pushEOB(stream, LL_codes);
```

## LZSS.hpp
This file houses the _LZSSEncoder 2_ class used by both block types. The class has the following private data members:
* buffer: an array of u8 which functions as window and look ahead
* output_buffer: vector of [Code objects](#code-structure) to be outputted
* curr_idx: the next idx to be encoded
* history_idx: the last index of our history
* bytes_processed: the next idx to add to look ahead and the number of bytes processes from the input data block

The class has a public _Encode_ member which takes as input the block to encode. The function first fills the buffer with sufficient lookahead. Then begins to process the data by looking for back references. The _getBackReference_ function does this using a linear search algorithm, however to improve the performance and compression the buffer is confined to a size of 5000 (ie. the farthest back reference is 4743 indeces away).

### PackageMerge
This class in _prefix codes.hpp_ takes a probability vector as described above and runs the package merge algorithm on the probabilities to assign each symbol and appropriate code length. The second parameter _num symbols_ allows the function to generate a vector of code lengths with one entry per symbol (ie. 288 for LL codes).
This vector is then returned by the function.

The algorithm used was derived from an example in the following article: https://experiencestack.co/length-limited-huffman-codes-21971f021d43

### DynamicCodes
This class in _prefix codes.hpp_ picks up where package merge left off. The constructor takes as input a vector of code lengths and uses the _construct canonical code_ function provided in the block 2 starter code to generate the vector of code sequences as 32 bit integers.

The _DynamicCodes_ class also has a member function _getCodeSequence_ very similar to that of the LLCodesBlock_1 and DistanceCodesBlock_1 classes. This function takes as input a symbol expresses as a 32 bit integer and returns the symbols dynamic encoding as a vector of bools.

# Advanced Features
* [Package Merge Algorithm](#packagemerge) to generate bounded code lengths


# Citations
* https://experiencestack.co/length-limited-huffman-codes-21971f021d43