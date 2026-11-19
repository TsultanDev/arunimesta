import aru;

int main() {
    auto window = aru::window::Window::create("Arunimesta", 800, 600).valueOrUnexpected([](aru::window::Error&& err){
        aru::standard::println("Error! : {}", err);
    });
    auto s16 = aru::standard::String<char16_t>::from(u"Hello");
    auto s16_2 = aru::standard::String<char16_t>::from("Hello");
    aru::standard::println("String: {}", s16);

    return 0;
}
