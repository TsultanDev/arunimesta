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
        framebuffers.push_back(framebuffer.move());
    }

    auto command_pool = unwrap(device.createCommandPool(queue));
    auto command_buffers = unwrap(device.allocateCommandBuffers(command_pool, swapchainImages.size()));

    while (window.isOpen()) {
        window.event_loop();
        uint32_t image_index = 0;

        aru::rhi::CommandBuffer& command_buffer = command_buffers[image_index];
        device.acquire([...]() mutable {
            //mulai waitFence, AcquireImage, resetfence, dan resetCommandBuffer sebelum closure dimulai,
            // fence dan semaphore (synchornizer) untuk sekarang disimpan di device saja.
            // frame_index disimpan di device bersama fence dan semaphore (syncronizer).

            command_buffer.record([...](){
                //otomaris memulai vkBeginCommandBuffer
                render_pass.render([/*Warna float( ex: 0.3f, 0.3f, 0.3f, 1.0f) */](){
                    //otomatis memulai vkCmdBeginRenderPass

                    // otomatis memanggil vkCmdEndRenderPass
                });
                //otomatis memanggil vkEndCommandBuffer
            });

            queue.submit(command_buffer); //untuk sekarang 1 command buffer dulu
            queue.present();
        });

    }

    return 0;
}
