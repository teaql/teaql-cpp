#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;
namespace req = teaql::request;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void helper_retention_contracts() {
    GlobalContext global;
    Ref<Value> value;
    Ref<Expr> filter;
    Ref<String> text;
    Ref<SmartList<Value>> rows;
    {
        auto user = global.make_user_context();
        auto source = user->memory_resource();
        auto destination = global.memory_resource();
        auto& json =
            user->make<Json>(*Json::parse(source, R"({"id":["  retained text  ",{"id":7}]})"));
        value = user->make_global<Value>(req::dynamic_json_value_to_teaql_value(destination, json));
        auto& values = user->make<Vector<Value>>();
        values.emplace_back(String(8192, 't', source));
        auto& retained_text = global.make<String>(
            req::required_text(destination, req::FieldOperator::Contain, values, 0));
        text = user->make_global<String>(retained_text);
        auto live = Expr::gt(source, "version", std::int64_t{0});
        Expr original(source, Expr::Kind::And);
        original.args.emplace_back(live.clone(source));
        original.args.emplace_back(Expr::eq(source, "private", values[0]));
        auto removed = req::remove_default_live_filter(destination,
                                                       std::optional<Expr>(original.clone(source)));
        check(removed && removed->kind == Expr::Kind::Binary &&
                  removed->memory_resource() == destination,
              "filter removal snapshots retained branches into the requested domain");
        filter = user->make_global<Expr>(*removed);
        rows = user->make_global<SmartList<Value>>();
        auto& list = user->get(rows);
        list.emplace_back(req::required_value(destination, req::FieldOperator::Equal, values, 0));
        auto& facets = user->make<std::pmr::map<String, SmartList<CompactRow>, std::less<>>>();
        Record record(source);
        record.emplace("label", Value(source, String(8192, 'f', source)));
        Vector<CompactRow> data(source);
        data.emplace_back(record);
        SmartList<CompactRow> facet(std::allocator_arg, Allocator(source), std::move(data));
        facets.emplace(String(4096, 'k', source), std::move(facet));
        req::attach_facets(list, facets);
        check(std::get<String>(values[0].data).size() == 8192 && original.args.size() == 2 &&
                  facets.size() == 1,
              "borrowed helper inputs remain available and unmodified after retention");
    }
    check(global.live_user_bytes() == 0, "actual source User deletion reclaims all helper inputs");
    auto next = global.make_user_context();
    auto owner = global.memory_resource();
    const auto& dynamic = next->get(next->rebind_global(value));
    const auto& dynamic_list = std::get<Value::List>(dynamic.data);
    check(dynamic_list.get_allocator().resource() == owner &&
              dynamic_list[0].try_text() == "retained text" && dynamic_list[1].try_i64() == 7,
          "dynamic JSON recursively retains exact values and destination allocation");
    const auto& expression = next->get(next->rebind_global(filter));
    check(expression.memory_resource() == owner &&
              expression.args[1].literal.try_text()->size() == 8192 &&
              std::get<String>(expression.args[1].literal.data).get_allocator().resource() == owner,
          "retained filtered expression contains only Global-owned nested data");
    check(next->get(next->rebind_global(text)).size() == 8192,
          "required text remains complete after source deletion");
    const auto& list = next->get(next->rebind_global(rows));
    const auto& [key, facet] = *list.facets.begin();
    check(key.size() == 4096 && key.get_allocator().resource() == owner &&
              facet.memory_resource() == owner && facet.is_loaded &&
              facet[0].get("label")->try_text()->size() == 8192,
          "facet attachment explicitly copies foreign keys, rows and nested strings into Global");
}

static void diagnostic_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    Vector<Value> empty(owner);
    try {
        (void)req::field_operator_expr(owner, "field", req::FieldOperator::Between, empty);
        throw std::runtime_error("Missing Between operands were accepted");
    } catch (const std::invalid_argument& error) {
        check(std::string_view(error.what()) == "Between requires value at index 0",
              "Between validates lower before upper, preserving original error precedence");
    }
    Vector<Value> wrong(owner);
    wrong.emplace_back(std::uint64_t{UINT64_MAX});
    try {
        (void)req::required_text(owner, req::FieldOperator::Contain, wrong, 0);
        throw std::runtime_error("Non-text field value was accepted");
    } catch (const std::invalid_argument& error) {
        check(std::string_view(error.what()) ==
                  "Contain requires text value, got U64(18446744073709551615)",
              "wrong-kind diagnostics preserve operator and complete original Value Debug");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        helper_retention_contracts();
        diagnostic_contracts();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
