#include <expected>
#include <print>
import aru;

int main() {
    auto app = aru::Application::init("Hello", 1);
    if (!app) {
        std::println(stderr, "App exit unsuccessfully!!");
        return 1;
    }

    app->printInfo();

    auto window = aru::window::Window("Arunimesta", 800, 600);
    while (window.event_loop()) {

    }

    std::println(stdout, "App exit successfully!!");
    return 0;
}
