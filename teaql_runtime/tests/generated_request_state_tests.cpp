#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <iostream>
using namespace cpp_dynamic_json;
using Query = EntityQuery<Order, cpp_dynamic_json::DataServiceExecutor>;

template <class T>
concept CanConsume = requires(T&& value) { std::forward<T>(value).intoQuery(); };
template <class T>
concept CanInspect = requires(const T& value) {
    value.queryOptions();
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
        check(options.memory_resource() == user_context->memory_resource() && options.comment &&
                  options.comment->get_allocator().resource() == user_context->memory_resource(),
              "Option snapshots propagate the source allocator");
        check(query.relationSelections().empty() && query.relationFilters().size() == 1 &&
                  query.childEnhancements().size() == 1,
              "Source views retain distinct collections");
        auto independent = query;
        query.comment("Replacement intent");
        check(*options.comment == "Long independently retained request comment" &&
                  *independent.queryOptions().comment ==
                      "Long independently retained request comment",
              "Snapshots and copied requests remain independent of later mutations");
        retained = &global.make<request::QueryOptions>(options);
        auto raw = std::move(independent).intoQuery();
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

// Result markers have no instances or allocation requirements. The source
// request permits arbitrary markers; its public execution facade requires Entity.
struct ResultMarker {
    ResultMarker() = delete;
    ResultMarker(const ResultMarker&) = delete;
    ~ResultMarker() = delete;
};

template <class T>
concept CanList =
    requires(const T& q, UserContext& user_context) { q.executeForList(user_context); };
template <class T>
concept CanRows =
    requires(const T& q, UserContext& user_context) { q.executeForRows(user_context); };
template <class T>
concept CanStream =
    requires(const T& q, UserContext& user_context) { q.executeForStream(user_context); };
template <class T>
concept CanFirst =
    requires(const T& q, UserContext& user_context) { q.executeForFirst(user_context); };
template <class T>
concept CanOne = requires(const T& q, UserContext& user_context) { q.executeForOne(user_context); };
template <class T>
concept CanPage =
    requires(const T& q, UserContext& user_context) { q.executeForPage(user_context, 0, 1); };
template <class T>
concept CanCount =
    requires(const T& q, UserContext& user_context) { q.executeForCount(user_context); };
template <class T>
concept CanExists =
    requires(const T& q, UserContext& user_context) { q.executeForExists(user_context); };
template <class T>
concept CanCreate = requires(const T& q, UserContext& user_context) { q.newEntity(user_context); };

using MarkerQuery = EntityQuery<Order, cpp_dynamic_json::DataServiceExecutor, true, ResultMarker>;
static_assert(!CanList<MarkerQuery> && !CanRows<MarkerQuery> && !CanStream<MarkerQuery> &&
              !CanFirst<MarkerQuery> && !CanOne<MarkerQuery> && !CanPage<MarkerQuery> &&
              !CanCount<MarkerQuery> && !CanExists<MarkerQuery> && !CanCreate<MarkerQuery>);
using GeneratedMarkerQuery = OrderRequest<true, ResultMarker>;
static_assert(!CanList<GeneratedMarkerQuery> && !CanRows<GeneratedMarkerQuery> &&
              !CanStream<GeneratedMarkerQuery> && !CanFirst<GeneratedMarkerQuery> &&
              !CanOne<GeneratedMarkerQuery> && !CanPage<GeneratedMarkerQuery> &&
              !CanCount<GeneratedMarkerQuery> && !CanExists<GeneratedMarkerQuery> &&
              !CanCreate<GeneratedMarkerQuery>);

static void marker_ownership_and_execution_boundary() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    setup(*user_context);
    auto Q = user_context->queries<query_factory>();
    auto source = Q.orders();
    auto marker = source.returnType<ResultMarker>();
    marker.comment("Independently retained arbitrary marker comment").limit(3);
    check(!source.query().comment_text && marker.query().slice->limit == 3,
          "Lvalue marker adaptation copies request state without constructing a marker");
    auto child = Q.people().returnType<std::uint64_t>();
    marker.enhanceChild(child).matchingAnyOf(child).selectCustomerWith(child);
    auto other = child.copyTo(*next_user_context);
    rejected([&] {
        marker.enhanceChild(other);
    });
    auto moved = std::move(marker).returnType<std::uint64_t>();
    rejected([&] {
        (void)marker.query();
    });
    auto copy = moved.copyTo(*next_user_context);
    auto retained = moved.copyToGlobal(*user_context);
    {
        auto options = copy.queryOptions();
        check(copy.memory_resource() == next_user_context->memory_resource() &&
                  options.comment->get_allocator().resource() == copy.memory_resource(),
              "Marker copies propagate the explicit destination allocator");
        auto consumed = moved;
        auto core = std::move(consumed).intoQuery();
        check(!core.comment_text && core.child_enhancements.empty(),
              "Marker consumption keeps the source bare-core boundary");
        rejected([&] {
            (void)consumed.query();
        });
    }
    auto purposed = moved.purpose("Restore an entity result before execution");
    auto restored = std::move(purposed).returnType<Order>();
    static_assert(CanList<decltype(restored)> && CanRows<decltype(restored)> &&
                  CanCount<decltype(restored)> && CanCreate<decltype(restored)>);
    check(restored.query().entity == "Order" && restored.query().child_enhancements.size() == 1,
          "Result restoration retains source identity and complete query state");
    auto executable = source.returnType<ResultMarker>()
                          .comment("Count after restoring the entity result marker")
                          .purpose("Verify ordinary execution remains available")
                          .returnType<Order>();
    check(require_value(executable.executeForCount(*user_context).wait()) == 0,
          "Restoring an Entity result permits actual SQLite execution");
    user_context.reset();
    rejected([&] {
        (void)restored.query();
    });
    auto rebound = retained.rebindGlobal(*next_user_context);
    check(rebound.query().comment_text == copy.query().comment_text,
          "Explicit Global marker retention survives actual source Context destruction");
    next_user_context.reset();
    check(global.live_user_bytes() == 0, "Marker requests do not retain User memory");
    rejected([&] {
        (void)copy.query();
    });
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ownership_and_consumption();
        marker_ownership_and_execution_boundary();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
