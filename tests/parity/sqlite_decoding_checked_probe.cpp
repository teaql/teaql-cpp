#define main sqlite_decoding_historical_main
#include "sqlite_decoding_probe.cpp"
#undef main

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--profile") {
#ifndef NDEBUG
        std::cout << "checked\n";
#else
        std::cout << "unchecked\n";
#endif
        return 0;
    }
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid checked decoding fixture");
        }
        try {
            std::cout << evaluate(*user_context, *input).dump(r) << '\n';
        } catch (const std::out_of_range& error) {
            if (std::string_view(error.what()) != "attempt to subtract with overflow") {
                throw;
            }
            Json::Object output(r);
            output.emplace("checked_failure", Json(r, "arithmetic_overflow"));
            output.emplace("message", Json(r, error.what()));
            std::cout << Json(r, output).dump(r) << '\n';
        }
    }
}
