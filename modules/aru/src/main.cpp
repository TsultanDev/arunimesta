
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
    auto adapter = instance.requestAdapter(surface).valueOrUnexpected([](aru::rhi::Error){

    });
    auto device = adapter.createDevice(adapter).valueOrUnexpected([](aru::rhi::Error){

    });
    auto queue = device.requestQueue(aru::rhi::QueueFlags::Graphics | aru::rhi::QueueFlags::Present).valueOrUnexpected([](aru::rhi::Error){

    });
    aru::standard::println("Aru RHI: queue ready (family {})", queue.familyIndex());

    while (window.isOpen()) {
        window.event_loop();
    }


    return 0;
}
