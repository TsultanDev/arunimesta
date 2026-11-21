import aru;

#include <exception>
#include <print>
#include <utility>
#include <vector>

int main() {
    // Pola demo: hentikan program sambil mencetak error bila expected kosong.
    auto unwrap = [](auto&& result) {
        if (!result) {
            std::println("Aru error: {}", result.error());
            std::terminate();
        }
        return std::move(*result);
    };

    auto window = unwrap(aru::window::Window::create("Arunimesta", 800, 600));
    auto instance = unwrap(aru::rhi::Instance::create("Arunimesta"));
    auto surface = unwrap(instance.createSurface(window));
    auto adapter = unwrap(instance.requestAdapter(surface));
    auto device = unwrap(adapter.createDevice(adapter));
    auto queue = unwrap(device.requestQueue(
        aru::rhi::QueueFlags::Graphics | aru::rhi::QueueFlags::Present
    ));

    auto swapchain = unwrap(device.createSwapchain(window.windowWidth(), window.windowHeight()));
    auto swapchainImages = unwrap(device.createSwapchainImages(swapchain));
    auto renderPass = unwrap(device.createRenderPass(swapchain));

    std::vector<aru::rhi::Framebuffer> framebuffers;
    framebuffers.reserve(swapchainImages.size());
    for (const auto& image : swapchainImages) {
        auto framebuffer = unwrap(device.createFramebuffer(renderPass, image));
        framebuffers.push_back(framebuffer.move()); // pindahkan handle, jangan copy.
    }

    std::println("Aru RHI: queue ready (family {})", queue.familyIndex());
    std::println("Aru RHI: framebuffers ready ({})", framebuffers.size());

    while (window.isOpen()) {
        window.event_loop();
    }

    return 0;
}
