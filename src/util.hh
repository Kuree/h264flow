//
// Created by keyi on 12/23/17.
//

#ifndef H264FLOW_UTIL_HH
#define H264FLOW_UTIL_HH

#include <stdexcept>

class NotImplemented : public std::logic_error
{
public:
    NotImplemented(std::string func_name) :
            std::logic_error(func_name + " not yet implemented") { };
};

/**
 Calculate the log base 2 of the argument, rounded up.
 Zero or negative arguments return zero
 Idea from http://www.southwindsgames.com/blog/2009/01/19/fast-integer-log2-function-in-cc/
 */
static uint64_t intlog2(uint64_t x)
{
    uint64_t log = 0;
    while ((x >> log) > 0)
    {
        log++;
    }
    if (x == 1u<<(log-1)) { log--; }
    return log;
}


#endif //H264FLOW_UTIL_HH
