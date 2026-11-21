
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

    auto swapchain = device.createSwapchain(window.windowWidth(), window.windowHeight()).valueOrUnexpected([](aru::rhi::Error){

    });
    auto swapchain_images = device.createSwapchainImages(swapchain).valueOrUnexpected([](aru::rhi::Error){

    });
    auto render_pass = device.createRenderPass(swapchain).valueOrUnexpected([](aru::rhi::Error){

    });
    auto framebuffers = aru::standard::Vector<aru::rhi::Framebuffer>::create(swapchain_images.length());
    for (const auto& image : swapchain_images){
        auto framebuffer = device.createFramebuffer(render_pass, image).valueOrUnexpected([](aru::rhi::Error){

        });
        framebuffers.push(framebuffer.move());
    }

    aru::standard::println("Aru RHI: queue ready (family {})", queue.familyIndex());
    aru::standard::println("Aru RHI: framebuffers ready ({})", framebuffers.length());

    while (window.isOpen()) {
        window.event_loop();
    }


    return 0;
}
