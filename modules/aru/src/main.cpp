#include <expected>
#include <print>
import aru;

int main(int argc, char** argv){
    auto result = aru::Application::init("Hello", 1).transform([](aru::Application&& app) -> void{
        app.printInfo();
    });
    if(!result){
        std::println(stdout, "App exit successfully!!");
    }
    return 0;
}
