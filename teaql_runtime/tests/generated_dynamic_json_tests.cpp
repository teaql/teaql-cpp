#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/core/request.hpp>
#include <iostream>

using namespace cpp_dynamic_json;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void check_expression_memory(const Expr& expression, Memory expected) {
    check(expression.memory_resource() == expected, "expression belongs to destination Context");
    for (const auto& argument : expression.args) {
        check_expression_memory(argument, expected);
    }
    if (expression.subquery_data) {
        const auto& nested = *expression.subquery_data;
        check(nested.entity.memory_resource() == expected &&
                  nested.query.memory_resource() == expected,
              "nested descriptor and query propagate the allocator");
        if (nested.query.predicate) {
            check_expression_memory(*nested.query.predicate, expected);
        }
    }
}

static void context_queries() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto Q = user_context->queries<query_factory>();
    auto request = [&] {
        String transient("{\"lines.order.customer.profile.note\":\"", r);
        transient.append(1024, 'x');
        transient += "\",\"_page\":2,\"_pageSize\":3}";
        auto value = Q.ordersMinimal().filterWithJson(transient);
        transient.assign(2048, 'y');
        return value;
    }();
    check(request.query().slice == Slice{3, 3}, "JSON input is parsed into owned query state");
    check_expression_memory(*request.query().predicate, r);
    auto retained = request.copyToGlobal(*user_context);
    auto foreign_user_context = global.make_user_context();
    auto copied = request.copyTo(*foreign_user_context);
    user_context.reset();
    check_expression_memory(*copied.query().predicate, foreign_user_context->memory_resource());
    bool closed = false;
    try {
        (void)request.query();
    } catch (const ReferenceError&) {
        closed = true;
    }
    check(closed, "ordinary request borrows expire after actual UserContext deletion");
    auto rebound = retained.rebindGlobal(*foreign_user_context);
    rebound.filterWithJson("{\"customer.name\":\"global added operand\"}");
    check_expression_memory(*rebound.query().predicate, global.memory_resource());
    check(rebound.query().slice == Slice{3, 3}, "explicit Global retention preserves pagination");
}

static void format_diagnostics() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto Q = user_context->queries<query_factory>();
    std::optional<GeneratedDynamicJsonFormatError> borrowed;
    std::optional<Ref<GeneratedDynamicJsonFormatError>> retained;
    try {
        Q.orders().filterWithJson("{\"code\":");
        throw std::logic_error("malformed JSON was accepted");
    } catch (const GeneratedDynamicJsonFormatError& error) {
        check(error.to_string(r) == "Input JSON format error: {\"code\":",
              "format error retains the complete source input");
        borrowed.emplace(error);
        retained.emplace(user_context->make_global<GeneratedDynamicJsonFormatError>(error));
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    auto destination = next_user_context->memory_resource();
    check(std::string_view(borrowed->what()) == "Input JSON format error",
          "what() remains safe without reading closed Context memory");
    bool closed = false;
    try {
        (void)borrowed->to_string(destination);
    } catch (const ReferenceError&) {
        closed = true;
    }
    check(closed, "borrowed diagnostic detects deleted Context before accessing text");
    const auto& global_error = next_user_context->get(next_user_context->rebind_global(*retained));
    check(global_error.to_string(destination) == "Input JSON format error: {\"code\":",
          "explicit diagnostic copy survives source Context deletion");
}

static void query_metadata_retention() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto Q = user_context->queries<query_factory>();
    auto retained = [&] {
        auto& query = user_context->make<SelectQuery>(Q.orders().query());
        const std::array<DynamicPropertyDefinitions::Field, 1> fields{{{"_metric", DataType::I64}}};
        auto definitions = DynamicPropertyDefinitions::from_fields(r, fields);
        check(bool(definitions), "readonly definitions must be valid");
        query.dynamic_property_definitions =
            detail::make_box<DynamicPropertyDefinitions>(r, definitions.value());
        auto child = Q.linesMinimal().query().clone(r);
        query.facets.emplace_back("byQuantity", "lines", child, false);
        query.relation_aggregates.emplace_back("lines", "lineCount", child, true);
        return user_context->make_global<SelectQuery>(query);
    }();
    user_context.reset();
    auto next_user_context = global.make_user_context();
    const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    auto gr = global.memory_resource();
    check(copy.dynamic_property_definitions->memory_resource() == gr &&
              copy.dynamic_property_definitions->data_type("_metric") == DataType::I64 &&
              copy.facets[0].memory_resource() == gr &&
              copy.facets[0].query.query.memory_resource() == gr &&
              copy.relation_aggregates[0].query.memory_resource() == gr,
          "nested facet, aggregate and readonly metadata survive only through explicit copies");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        context_queries();
        format_diagnostics();
        query_metadata_retention();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated dynamic JSON Context contracts passed\n";
}
