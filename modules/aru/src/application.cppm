module;
#include <cstdint>
#include <expected>
#include <memory>
#include <utility>
#include <print>

export module aru:application;

export namespace aru {
    template <typename T>
    class Handle {
    private:
        std::unique_ptr<T> pointer;

    public:
        Handle() = default;
        explicit Handle(std::unique_ptr<T> ptr) noexcept : pointer(std::move(ptr)) {}

        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;

        Handle(Handle&&) noexcept = default;
        Handle& operator=(Handle&&) noexcept = default;

        T* operator->() noexcept { return pointer.get(); }
        const T* operator->() const noexcept { return pointer.get(); }
    };

    struct Application {
        const char* name = nullptr;
        uint32_t version = 0;

        static std::expected<Handle<Application>, int> init(const char* name, uint32_t version);
        void printInfo() const;
    };
}

std::expected<aru::Handle<aru::Application>, int> aru::Application::init(const char* name, uint32_t version) {
    if (!name) {
        return std::unexpected(-1);
    }

    auto app = std::make_unique<Application>(Application{
        .name = name,
        .version = version
    });

    #ifndef NDEBUG
    std::println(stdout, "Aru: Application(created!!), name: {}, version: {}", name, version);
    #endif

    return Handle<Application>(std::move(app));
}

void aru::Application::printInfo() const {
    std::println(stdout, "Aru: Application(created!!)");
}