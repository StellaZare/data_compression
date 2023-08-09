#include "discrete_cosine_transform.hpp"

namespace helper{
    
    /* ----- Helper Functions - written by Bill ----- */
    //The floating point calculations we use while converting between 
    //RGB and YCbCr can occasionally yield values slightly out of range
    //for an unsigned char (e.g. -1 or 255.9).
    //Furthermore, we want to ensure that any conversion uses rounding
    //and not truncation (to improve accuracy).
    inline unsigned char round_and_clamp_to_char(double v){
        //Round to int 
        int i = (int)(v+0.5);
        //Clamp to the range [0,255]
        if (i < 0)
            return 0;
        else if (i > 255)
            return 255;
        return i;
    }

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
