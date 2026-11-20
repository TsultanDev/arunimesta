
import aru;

int main() {
    auto window = aru::window::Window::create("Arunimesta", 800, 600).valueOrUnexpected([](aru::window::Error&& err){
        aru::standard::println("Error! : {}", err);
    });



    auto s8 = aru::standard::String<char8_t>::from("Arunimesta");
    auto instance = aru::rhi::Instance::create(s8).valueOrUnexpected([](aru::rhi::Error){

    });
    auto surface = instance.createSurface(window).valueOrUnexpected([](aru::rhi::Error){

    });

    while (window.isOpen()) {
        window.event_loop();
    }


    return 0;
}
