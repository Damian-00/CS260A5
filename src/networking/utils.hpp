#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <chrono>
//#include <fmt/core.h>
//#include <fmt/ostream.h>
#include <ctime>
#include <array>

namespace CS260 {

    // Time utils
    using clock_t = std::chrono::high_resolution_clock;

    inline auto now()
    {
        return clock_t::now();
    }

    inline uint32_t now_ns()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(clock_t::now().time_since_epoch()).count();
    }

    inline uint32_t ms_since(clock_t::time_point const& moment)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(now() - moment).count();
    }

    /*	\fn PrintMessage
    \brief	Prints the given message if verbose is on.
    */
    inline void PrintMessage(const std::string& message, bool verbose)
    {
        if (verbose)
            std::cerr << message << std::endl;
    }

    inline std::string NowString()
    {
        //auto                  tp        = clock_t::now();
        //auto                  time      = std::chrono::to_time_t(tp);
        //auto                  timeLocal = std::localtime(&time);
        std::array<char, 256> str{};
        //strftime(str.data(), str.size() - 1, "%H:%M:%S", timeLocal);
        return std::string(str.data());
    }

}

#endif // __UTILS_HPP__