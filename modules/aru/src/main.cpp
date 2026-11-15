#include <expected>
#include <print>
import aru;

int main(int argc, char** argv){
    auto result = aru::Application::init("Hello", 1).transform([](aru::Application&& app) -> void{
        app.printInfo();
        auto window = aru::window::Window::create_window("Arunimesta", 800, 600).transform([](aru::window::Window&& window)-> void{
            while(window.event_loop()){

            }
        });
    });
    if(!result){
        std::println(stdout, "App exit unsuccessfully!!");
    }
    std::println(stdout, "App exit successfully!!");
    return 0;
}
