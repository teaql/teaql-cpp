#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <iostream>
using namespace cpp_dynamic_json;
using Query = EntityQuery<Order, cpp_dynamic_json::DataServiceExecutor>;

template <class T>
concept CanConsume = requires(T&& value) { std::forward<T>(value).intoQuery(); };
template <class T>
concept CanInspect = requires(const T& value) {
    value.queryOptions();
    value.coreQuery();
    value.relationFilters();
    value.relationSelections();
    value.childEnhancements();
};
static_assert(CanConsume<Query> && !CanConsume<Query&>);
static_assert(CanInspect<Query> &&
              !CanInspect<EntityQuery<Order, cpp_dynamic_json::DataServiceExecutor, true>>);

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

template <class F>
static void rejected(F&& action) {
    bool failed = false;
    try {
        std::forward<F>(action)();
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, "Expected checked access rejection");
}

static void ownership_and_consumption() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto Q = user_context->queries<query_factory>();
    auto query =
        Q.orders()
            .comment("Long independently retained request comment")
            .rawSql("Long application-controlled SQL text")
            .rawSqlFilter("Long independently retained SQL criterion")
            .createPropertyAs("computed", "Long independently retained property expression")
            .selectCodeRaw("Long independently retained projection expression")
            .enhanceChild(Q.people().comment("Long independently retained child comment"))
            .withCustomerMatching(
                Q.people().whoseNameIs("Long independently retained filter value"));
    using Request = decltype(query);
    static_assert(CanConsume<Request> && !CanConsume<Request&> && CanInspect<Request>);
    const request::QueryOptions* retained = nullptr;
    const SelectQuery* retained_core = nullptr;
    {
        auto options = query.queryOptions();
        auto bare = query.coreQuery();
        check(!bare.comment_text && !bare.raw_sql_text && bare.child_enhancements.empty(),
              "Bare snapshot separates source core from options");
        check(query.query().comment_text && query.query().raw_sql_text &&
                  !query.query().child_enhancements.empty(),
              "Core inspection does not mutate integrated execution state");
        check(options.memory_resource() == user_context->memory_resource() && options.comment &&
                  options.comment->get_allocator().resource() == user_context->memory_resource(),
              "Option snapshots propagate the source allocator");
        check(query.relationSelections().empty() && query.relationFilters().size() == 1 &&
                  query.childEnhancements().size() == 1,
              "Source views retain distinct collections");
        auto independent = query;
        query.comment("Replacement intent").limit(42);
        check(!bare.slice || bare.slice->limit != 42,
              "Bare snapshots remain independent of later mutations");
        check(*options.comment == "Long independently retained request comment" &&
                  *independent.queryOptions().comment ==
                      "Long independently retained request comment",
              "Snapshots and copied requests remain independent of later mutations");
        retained = &global.make<request::QueryOptions>(options);
        auto before_consume = independent.coreQuery();
        auto raw = std::move(independent).intoQuery();
        check(raw == before_consume, "Consuming and non-consuming core extraction agree");
        check(!raw.comment_text && !raw.raw_sql_text && raw.raw_sql_search_criteria.empty() &&
                  raw.dynamic_properties.empty() && raw.raw_projections.empty() &&
                  raw.child_enhancements.empty(),
              "Direct consuming conversion omits independent options and enhancements");
        check(raw.memory_resource() == user_context->memory_resource(),
              "Extracted core query retains its declared allocation scope");
        retained_core = &global.make<SelectQuery>(raw);
        rejected([&] {
            (void)independent.query();
        });
        rejected([&] {
            (void)independent.queryOptions();
        });
        auto complete_selection = query.selection();
        auto complete = std::move(complete_selection).into_query();
        check(complete.comment_text && complete.raw_sql_text &&
                  !complete.dynamic_properties.empty() && !complete.child_enhancements.empty(),
              "Selection conversion applies source metadata");
    }
    user_context.reset();
    check(global.live_user_bytes() == 0 && retained->comment &&
              *retained->comment == "Long independently retained request comment" &&
              retained_core->entity == "Order",
          "Explicit Global copies survive actual source User deletion");
    rejected([&] {
        (void)query.relationFilters();
    });
    rejected([&] {
        (void)query.childEnhancements();
    });
    rejected([&] {
        (void)query.relationSelections();
    });
    rejected([&] {
        (void)query.queryOptions();
    });
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ownership_and_consumption();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
