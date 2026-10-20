#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "../../tests/generated_relations/include/cpp_relations.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}
template <class Parent, class Child>
concept Enhance = requires(Parent& parent, const Child& child) {
    parent.enhanceChild(child);
    std::move(parent).enhanceChild(child);
};
static_assert(Enhance<cpp_dynamic_json::PersonRequest<false>, cpp_relations::SchoolRequest<false>>);
static_assert(!Enhance<cpp_dynamic_json::PersonRequest<false>, cpp_relations::SchoolRequest<true>>);

static void lifetime() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto other_Q = user_context->queries<cpp_relations::query_factory>();
    auto parent = Q.peopleMinimal();
    auto child = other_Q.schoolsMinimal()
                     .comment("long child enhancement intent copied to destination")
                     .rawSqlFilter("1=1");
    parent.enhanceChild(child);
    child.rawSqlFilter("1=0");
    check(parent.childEnhancements().size() == 1, "Cross-domain child captured");
    check(parent.childEnhancements()[0].query_options.raw_sql_search_criteria.size() == 1,
          "Child capture is independent");
    {
        auto foreign = global.make_user_context();
        auto foreign_Q = foreign->queries<cpp_relations::query_factory>();
        auto child_request = foreign_Q.schoolsMinimal();
        bool rejected = false;
        try {
            parent.enhanceChild(child_request);
        } catch (const ReferenceError&) {
            rejected = true;
        }
        check(rejected, "Foreign Context child rejects");
        auto raw = child_request.selection();
        rejected = false;
        try {
            parent.enhanceChild(raw);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Foreign core selection rejects");
        auto& copied = user_context->make<request::QuerySelection>(raw);
        parent.enhanceChild(copied);
    }
    auto retained = parent.copyToGlobal(*user_context);
    user_context.reset();
    bool expired = false;
    try {
        (void)parent.query();
    } catch (const std::exception&) {
        expired = true;
    }
    check(expired, "Ordinary parent expires");
    auto next_user_context = global.make_user_context();
    auto rebound = retained.rebindGlobal(*next_user_context);
    auto output = debug_string(next_user_context->memory_resource(), rebound.query());
    check(output.find("long child enhancement intent") != String::npos,
          "Global child options survive deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        lifetime();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
