import aru;

#include <array>
#include <cstdint>
#include <exception>
#include <print>
#include <utility>
#include <vector>

int main() {
    auto unwrap = [](auto&& result) {
        if (!result) {
            std::println("Aru error: {}", static_cast<std::uint32_t>(result.error()));
            std::terminate();
        }
        return std::move(*result);
    };

    // Untuk expected<void, Error> (frame/record/render/submit/present).
    auto check = [](auto&& result) {
        if (!result) {
            std::println("Aru error: {}", static_cast<std::uint32_t>(result.error()));
            std::terminate();
        }
    };

    auto window = unwrap(winaru::Window::create("Arunimesta", 800, 600));
    auto instance = unwrap(renaru::Instance::create("Arunimesta"));
    auto surface = unwrap(instance.createSurface(window));
    auto adapter = unwrap(instance.requestAdapter(surface));
    auto device = unwrap(adapter.createDevice(adapter));
    auto queue = unwrap(device.requestQueue(
        renaru::QueueFlags::Graphics | renaru::QueueFlags::Present
    ));

    auto swapchain = unwrap(device.createSwapchain(window.windowWidth(), window.windowHeight()));
    auto swapchainImages = unwrap(device.createSwapchainImages(swapchain));
    auto renderPass = unwrap(device.createRenderPass(swapchain));

    std::vector<renaru::Framebuffer> framebuffers;
    framebuffers.reserve(swapchainImages.size());
    for (const auto& image : swapchainImages) {
        auto framebuffer = unwrap(device.createFramebuffer(renderPass, image));
        framebuffers.push_back(std::move(framebuffer));
    }

    auto command_pool = unwrap(device.createCommandPool(queue));
    auto command_buffers = unwrap(device.allocateCommandBuffers(command_pool, swapchainImages.size()));

    constexpr std::array<float, 4> clearColor{0.05f, 0.05f, 0.05f, 1.0f};

    while (window.isOpen()) {
        // event_loop() bersifat blocking; pollEvents() hanya menguras pesan
        // yang sedang antre supaya loop render bisa berjalan tiap frame.
        window.pollEvents();
        if (window.shouldClose()) {
            break;
        }

        check(device.acquire([&]() mutable {
            // Device sudah menunggu fence, meng-acquire gambar, mereset fence,
            // dan menyimpan image_index/frame_index sebelum closure ini jalan.
            const std::uint32_t image_index = device.currentImageIndex();
            renaru::CommandBuffer& command_buffer = command_buffers[image_index];

            check(command_buffer.record([&]() {
                // record() otomatis reset + vkBeginCommandBuffer, lalu
                // vkEndCommandBuffer setelah closure render selesai.
                check(renderPass.render(clearColor, []() {
                    // render() otomatis vkCmdBeginRenderPass / vkCmdEndRenderPass.
                    // Perintah draw akan ditambahkan di sini.
                }));
            }));

            check(queue.submit(command_buffer));
            check(queue.present());

            // Device otomatis menaikkan frame_index (modulo MaxFramesInFlight)
            // setelah closure acquire selesai.
        }));
    }

    device.waitIdle();
    return 0;
}
