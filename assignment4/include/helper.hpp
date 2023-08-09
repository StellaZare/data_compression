#include "discrete_cosine_transform.hpp"

namespace helper{
    
    /* ----- Helper Functions - written by Bill ----- */
    //Convenience function to wrap around the nasty notation for 2d vectors
    //Convenience function to wrap around the nasty notation for 2d vectors
    template<typename T>
    std::vector<std::vector<T> > create_2d_vector(unsigned int outer, unsigned int inner){
        std::vector<std::vector<T> > V {outer, std::vector<T>(inner,T() )};
        return V;
    }

    /* ----- My Helpers ----- */
    dct::Quality get_quality(std::string input_quality){
        if(input_quality == "low")
            return dct::Quality::low;
        else if(input_quality == "medium")
            return dct::Quality::medium;
        else if(input_quality == "high")
            return dct::Quality::high;
        else
            return dct::Quality::ERROR;
    }
}
