#include <teaql/runtime/entity_expression.hpp>
#include "fixture_value.hpp"
#include <iostream>

static void ownership() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    std::string expected;
    auto references = [&] {
        String root("Root_Name(id=(", r);
        root.append(4096, 'x');
        root += ") trailing";
        String path("parent.", r);
        path.append(2048, 'z');
        path += ".name";
        auto& diagnostic = user_context->make<ExpressionDiagnostic>(root, "parent", path);
        expected = std::string(diagnostic.to_json(r).dump(r));
        return std::pair{user_context->make_global<ExpressionDiagnostic>(diagnostic),
                         user_context->make_ref<ExpressionDiagnostic>(diagnostic)};
    }();
    user_context.reset();
    auto next_user_context = global.make_user_context();
    bool expired = false;
    try {
        (void)next_user_context->get(references.second);
    } catch (const ReferenceError&) {
        expired = true;
    }
    if (!expired) {
        throw std::runtime_error("expired diagnostic reference resolved");
    }
    const auto& retained =
        next_user_context->get(next_user_context->rebind_global(references.first));
    if (retained.memory_resource() != global.memory_resource() ||
        std::string_view(retained.to_json(next_user_context->memory_resource())
                             .dump(next_user_context->memory_resource())) !=
            std::string_view(expected)) {
        throw std::runtime_error("retained diagnostic differs after User destruction");
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            ownership();
            std::pmr::set_default_resource(previous);
            return 0;
        }
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("invalid diagnostic text fixture");
            }
            auto& diagnostic =
                user_context->make<ExpressionDiagnostic>(text(field(*input, "root")),
                                                         text(field(*input, "node")),
                                                         text(field(*input, "path")));
            auto encoded = diagnostic.to_json(r).dump(r);
            auto decoded = Json::parse(r, encoded);
            if (!decoded || decoded->dump(r) != encoded) {
                throw std::runtime_error("diagnostic JSON roundtrip failed");
            }
            std::cout << encoded << '\n';
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
