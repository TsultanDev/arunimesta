module;
#include <cstdio>
#include <format>
#include <print>

export module aru.standard:print;

export namespace aru{
    namespace standard{
        template<typename... Args>
        void println(std::format_string<Args...> fmt, Args&&... args);
        void println();
        template<typename... Args>
        void eprintln(std::format_string<Args...> fmt, Args&&... args);
        void eprintln();
    }
}

template<typename... Args>
void aru::standard::println(std::format_string<Args...> fmt, Args&&... args){
    #ifndef NDEBUG
    std::println(stdout, fmt, std::forward<Args>(args)...);
    #endif
}
void aru::standard::println(){
    std::println(stdout);
}

template<typename... Args>
void  aru::standard::eprintln(std::format_string<Args...> fmt, Args&&... args){
    #ifndef NDEBUG
    std::println(stderr, fmt, std::forward<Args>(args)...);
    #endif
}
void aru::standard::eprintln(){
    std::println(stderr);
}
