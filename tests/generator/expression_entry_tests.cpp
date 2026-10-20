#include <teaql/runtime/entity_expression.hpp>
#include <iostream>
#include <optional>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void inspect(const ExpressionDiagnostic& diagnostic) {
    check(diagnostic.root == "Class(id=10)" && diagnostic.failed_node == "name" &&
              diagnostic.attempted_path == "platform.name",
          "Native API metadata must not rename canonical entity or field identity");
    check(diagnostic.original_expr_with_broken_point ==
              "E::modelClass(id=10).getPlatform().getName()<broken>",
          "The generated native entry must survive nested expression traversal");
    check(diagnostic.suggested_fix == "selectName()",
          "Entry metadata must not alter the canonical field repair");
}

static void inspect_missing(const RuntimeExpression<std::int64_t>& expression) {
    try {
        (void)expression.unwrap();
    } catch (const ExpressionNotLoadedError& error) {
        inspect(error.diagnostic());
        return;
    }
    throw std::runtime_error("Expected the preserved NotLoaded result");
}

static void root_metadata_lifetime() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto destination = global.make_user_context();
    std::optional<RuntimeExpression<std::int64_t>> expired;
    std::optional<Ref<ExpressionNotLoadedError>> retained_error;
    auto copied = [&] {
        auto r = user_context->memory_resource();
        using LegacyFactory =
            RuntimeExpression<std::int64_t> (*)(eval::EvalResult<std::int64_t>, std::string_view);
        LegacyFactory original_factory = &RuntimeExpression<std::int64_t>::from_result;
        auto ordinary =
            original_factory(eval::EvalResult<std::int64_t>::value(r, 11), "Legacy(id=1)");
        check(ordinary.unwrap() == 11, "The original exact factory signature remains usable");
        String description("Class(id=10)", r);
        auto expression = RuntimeExpression<std::int64_t>::from_result(
            eval::EvalResult<std::int64_t>::not_loaded(r, "name", "platform.name"),
            description,
            StaticString{"modelClass"});
        description.assign("NotTheCapturedRoot(id=99)");
        expired = expression;
        auto mapped = expression.map([](std::int64_t) {
            throw std::logic_error("Missing callback must not run");
            return std::int64_t(0);
        });
        try {
            (void)mapped.unwrap();
        } catch (const ExpressionNotLoadedError& error) {
            inspect(error.diagnostic());
            retained_error = user_context->make_global<ExpressionNotLoadedError>(error);
        }
        check(bool(retained_error), "The missing expression must retain a structured diagnostic");
        auto skipped = expression.andThen("ignored", [r](std::int64_t) {
            throw std::logic_error("Missing traversal callback must not run");
            return RuntimeExpression<std::int64_t>::null(r);
        });
        inspect_missing(skipped);
        auto value = RuntimeExpression<std::int64_t>::from_result(
            eval::EvalResult<std::int64_t>::value(r, 7),
            "Class(id=10)",
            StaticString{"modelClass"});
        auto nested = value
                          .map([](std::int64_t source) {
                              return source + 1;
                          })
                          .andThen("platform", [r](std::int64_t) {
                              return RuntimeExpression<std::int64_t>::not_loaded(r, "name", "name");
                          });
        inspect_missing(nested);
        return expression.copyTo(*destination);
    }();
    user_context.reset();
    bool rejected = false;
    try {
        (void)expired->unwrap();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Static API names must not extend Context-owned expression state");
    inspect_missing(copied);
    const auto& retained = destination->get(destination->rebind_global(*retained_error));
    inspect(retained.diagnostic());
    auto materialized = retained.diagnostic().to_json(destination->memory_resource());
    check(materialized.dump(destination->memory_resource()).find("modelClass") != String::npos,
          "Explicit retained diagnostics preserve their static entry metadata");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        root_metadata_lifetime();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "static_name_bytes=" << sizeof(StaticString)
              << " root_description_bytes=" << sizeof(String) << '\n';
}
