module;
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <utility>
#include <print>

export module aru:application;

namespace aru {
    export enum class ApplicationError {
        Unknown
    };

    struct Application_T;
    export class Application;
}

struct aru::Application_T {
    std::string name = "";
    uint32_t version = 0;
};

class aru::Application {
    public:
        static std::expected<Application, ApplicationError> init(std::string name, uint32_t version);
        void printInfo() const;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        Application(Application&&) noexcept = default;
        Application& operator=(Application&&) noexcept = default;

        void* operator new(size_t) = delete;
        void* operator new[](size_t) = delete;

    private:
        Application(std::unique_ptr<Application_T> app);
        std::unique_ptr<Application_T> app;
};

aru::Application::Application(std::unique_ptr<aru::Application_T> app) : app(std::move(app)) {
}

std::expected<aru::Application, aru::ApplicationError> aru::Application::init(std::string name, uint32_t version) {
    auto app = Application(std::make_unique<Application_T>(Application_T{
        .name = std::move(name),
        .version = version,
    }));

    #ifndef NDEBUG
    std::println(stderr, "Aru: Application(created!!)");
    std::println(stderr, "Aru: Application(created!!), name: {}, version: {}", app.app->name, app.app->version);
    #endif

    return app;
}

void aru::Application::printInfo() const {
    if (!this->app) {
        return;
    }
    std::println(stdout, "Aru: Application(created!!), name: {}, version: {}", this->app->name, this->app->version);
}