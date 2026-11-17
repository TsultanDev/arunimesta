module;
#include <cstdio>
#include <print>
#include <utility>

export import aru.std:print;

export namespace aru{
    namespace standard{
        template<typename... Args>
        inline void println(std::format_string<Args...> fmt, Args&&... args){
            #ifndef NDEBUG
            std::println(stdout, fmt, std::forward<Args>(args)...);
            #endif
        }
        inline void println(){
            std::println(stdout);
        }
    }
}
