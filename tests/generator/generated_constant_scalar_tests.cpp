#include "../generated_constants_scalar/include/cpp_constants_scalar.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void scalar_initialization() {
    using namespace cpp_constants_scalar;
    GlobalContext global;
    auto user_context = global.make_user_context();
    configureRuntime(*user_context);
    check(static_cast<bool>(user_context->ensure_schema().wait()), "scalar initialization failed");
    auto Q = user_context->queries<query_factory>();
    auto load = [&]() -> auto& {
        return Q.priorities()
            .selectSelfFields()
            .orderByIdAsc()
            .comment("what: inspect scalar constants")
            .purpose("why: validate generated initialization")
            .executeForList(*user_context);
    };
    const auto& initial = load().wait();
    check(static_cast<bool>(initial) && initial.value().len() == 2, "expected two constants");
    const auto& first = *initial.value().get(0);
    const auto& second = *initial.value().get(1);
    auto r = user_context->memory_resource();
    auto expected_json = Json::parse(r, "\"jsonMe()\"");
    check(!first.active() && second.active(), "boolean constant values changed");
    check(first.rank() == -7 && first.weight() == 2.5 && first.ratio() == 1.25f,
          "signed and floating constants changed");
    check(first.cost() == Decimal::parse("12.50").value() &&
              first.day() == Date::parse("1970-01-01").value(),
          "Decimal or source epoch-date default changed");
    check(first.payload() && expected_json && *first.payload() == *expected_json &&
              first.note() == std::optional<String>{String("First note", r)} &&
              first.optionalRank() == std::optional<std::int64_t>{0},
          "JSON and optional constants changed");
    check(first.version() == 1 && second.version() == 1, "initial versions changed");
    check(static_cast<bool>(user_context->ensure_schema().wait()), "no-op initialization failed");
    const auto& unchanged = load().wait();
    check(unchanged && unchanged.value().get(0)->version() == 1 &&
              unchanged.value().get(1)->version() == 1,
          "no-op initialization must preserve versions");
    auto transport = user_context->require_resource<SqliteMutationExecutor>();
    check(
        transport &&
            sqlite3_exec(
                transport.value().get().connection().native_handle(),
                "UPDATE priority_data SET active=1,payload=NULL,note=NULL,optional_rank=NULL WHERE id=1",
                nullptr,
                nullptr,
                nullptr) == SQLITE_OK,
        "stored-state corruption failed");
    check(static_cast<bool>(user_context->ensure_schema().wait()), "scalar reconciliation failed");
    const auto& repaired = load().wait();
    check(repaired && repaired.value().get(0)->version() == 2 &&
              repaired.value().get(1)->version() == 1 && !repaired.value().get(0)->active() &&
              repaired.value().get(0)->payload() == first.payload() &&
              repaired.value().get(0)->optionalRank() == first.optionalRank(),
          "reconciliation must restore values and increment only the changed row");
    auto expression = E::priority(*repaired.value().get(0)).getPayload();
    auto next_user_context = global.make_user_context();
    auto copied = expression.copyTo(*next_user_context);
    auto copied_expected = Json::parse(next_user_context->memory_resource(), "\"jsonMe()\"");
    user_context.reset();
    bool expired = false;
    try {
        (void)expression.unwrap();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired && copied_expected && copied.unwrap() && *copied.unwrap() == *copied_expected,
          "Context-bound JSON expressions require explicit destination copies");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scalar_initialization();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout
        << "Generated scalar constant C++ integration passed; original crate remains uncompilable\n";
}
