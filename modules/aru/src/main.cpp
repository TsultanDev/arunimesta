#include <cstdlib>
#include <expected>
#include <utility>
import aru;

int main(int argc, char** argv){
    auto result = aru::Application::init("Hello", 1).transform([](aru::Application&& app) -> void{
        app.printInfo();
    });
    return 0;
}
