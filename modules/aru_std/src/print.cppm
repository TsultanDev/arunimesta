module;
#include <cstdio>
#include <print>

export module aru.std:print;

export namespace aru{
    namespace standard{
        template<typename... Args>
        inline void println(std::format_string<Args...> fmt, Args&&... args);
        inline void println();
        template<typename... Args>
        inline void eprintln(std::format_string<Args...> fmt, Args&&... args);
        inline void eprintln();
    }
}

template<typename... Args>
inline void aru::standard::println(std::format_string<Args...> fmt, Args&&... args){
    #ifndef NDEBUG
    std::println(stdout, fmt, std::forward<Args>(args)...);
    #endif
}
inline void aru::standard::println(){
    std::println(stdout);
}

template<typename... Args>
inline void  aru::standard::eprintln(std::format_string<Args...> fmt, Args&&... args){
    #ifndef NDEBUG
    std::println(stderr, fmt, std::forward<Args>(args)...);
    #endif
}
inline void aru::standard::eprintln(){
    std::println(stderr);
}
