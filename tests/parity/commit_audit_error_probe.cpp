#include <teaql/runtime/error_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("invalid error fixture");
            }
            auto error = RuntimeError::event(r, text(field(*input, "message")));
            for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(*input, "depth").data);
                 ++i) {
                error = RuntimeError::audit_after_commit(r, error);
            }
            Json::Object out(r);
            out.emplace(String("display", r), Json(r, error.to_string(r)));
            out.emplace(String("debug", r), Json(r, debug_string(r, error)));
            DebugOptions options;
            options.pretty = true;
            out.emplace(String("pretty", r), Json(r, debug_string(r, error, options)));
            out.emplace(String("source_none", r), Json(r, error.source() == nullptr));
            std::cout << Json(r, out).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
