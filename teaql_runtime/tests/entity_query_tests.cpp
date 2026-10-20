#include "../../examples/generated/include/cpp_examples.hpp"
#include "../../tests/support/entity_graph_plan_fixture.hpp"
#include <teaql/runtime/entity_query.hpp>
#include <iostream>

using namespace teaql;
using cpp_examples::Product;
using ExampleExecutor = cpp_examples::DataServiceExecutor;
using Query = EntityQuery<Product, ExampleExecutor>;
using Purposed = EntityQuery<Product, ExampleExecutor, true>;

template <class Q>
concept ExecutableList =
    requires(Q& query, UserContext& user_context) { query.executeForList(user_context); };
template <class Q>
concept ExecutableOne =
    requires(Q& query, UserContext& user_context) { query.executeForOne(user_context); };
template <class Q>
concept ExecutableStream =
    requires(Q& query, UserContext& user_context) { query.executeForStream(user_context); };
static_assert(!ExecutableList<Query> && !ExecutableOne<Query> && !ExecutableStream<Query>);
static_assert(ExecutableList<Purposed> && ExecutableOne<Purposed> && ExecutableStream<Purposed>);
static_assert(!std::default_initializable<Query>);
static_assert(!std::default_initializable<Purposed>);
static_assert(requires(UserContext& user_context) {
    user_context.queries<cpp_examples::query_factory>().orderLines();
});

template <class Factories>
concept ContextFactories =
    requires(Factories& factory, UserContext& user_context) { factory.orderLines(user_context); };
static_assert(!ContextFactories<cpp_examples::query_factory>);

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void compact_list_trace_and_intent() {
    GlobalContext global;
    for (bool capture : {false, true}) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        user_context->set_sql_log_options(SqlLogOptions{.select = capture, .mutation = false});
        auto& executor = user_context->make<graph_plan_fixture::Executor>();
        executor.rows.emplace_back();
        executor.rows.back().emplace(String("id", r), Value(r, std::uint64_t{7}));
        auto& service = EntityDataService<graph_plan_fixture::Executor>::for_executor(
            *user_context, "Parent", *user_context, executor);
        SelectQuery query(std::allocator_arg, Allocator(r), "Parent");
        query.comment("Observe compact list")
            .order_asc("id")
            .limit(1)
            .optimize_for_continuous_page_fetch();
        PurposedSelectQuery request(
            std::allocator_arg, Allocator(r), query, "Verify compact trace ownership");
        auto before = detail::clone_in(r, request.as_query().trace_chain);
        auto& result = service.fetch_smart_list(request).wait();
        check(result && result.value().len() == 1 && executor.requests.size() == 1,
              "compact SmartList uses one direct provider call even with a pagination hint");
        const auto& physical = executor.requests.back();
        check(physical.query.trace_chain.empty() &&
                  physical.trace_chain.size() == (capture ? before.size() : 0) &&
                  physical.comment() == "Observe compact list" &&
                  physical.purpose() == "Verify compact trace ownership" &&
                  physical.capture_execution_metadata == capture,
              "physical trace extraction preserves independent validated intent");
        check(!before.empty() && request.as_query().trace_chain == before,
              "execution does not clear the reusable query's original trace");
        executor.pending = true;
        auto& pending = service.fetch_smart_list(request);
        check(!pending.poll(), "compact list forwarding preserves suspension");
        pending.cancel();
        check(executor.cancelled == 1, "compact list cancellation reaches its provider once");
    }
}

template <class F>
static void rejects(F&& action, const char* message) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, message);
}

static void seed(UserContext& user_context, std::string_view name) {
    auto& product = Query(user_context)
                        .comment("Construct query fixture")
                        .purpose("Seed product")
                        .newEntity(user_context);
    product.updateName(name);
    auto& result = AuditedSaveExt<Product>::save(std::move(product).audit_as("Seed query fixture"),
                                                 user_context);
    cpp_examples::require_value(result.wait());
}

static Purposed product_request(UserContext& user_context) {
    return Query(user_context).orderAsc("id").comment("Read products").purpose("Inspect products");
}

static auto named_products(cpp_examples::ProductRequest<> request, std::string_view name) {
    return std::move(request).withNameIs(name);
}

static void construction_and_execution() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto executor_ref = cpp_examples::setup(*user_context);
    auto query = product_request(*user_context);
    auto copy = query;
    auto nested = [&] {
        auto child = Query(*user_context).whereEqual("name", "owned child description");
        return EntityQuery<cpp_examples::OrderLine, ExampleExecutor>(*user_context)
            .selectRelation("product", child);
    }();
    auto nested_copy = nested;
    nested = EntityQuery<cpp_examples::OrderLine, ExampleExecutor>(*user_context);
    const auto& relation = nested_copy.query().relations.front();
    check(relation.query && relation.query->memory_resource() == nested_copy.memory_resource() &&
              relation.query->predicate.has_value(),
          "nested selections own a snapshot after child wrapper destruction");
    seed(*user_context, "first");
    seed(*user_context, "second");
    std::string input_name = "first";
    auto processed = [&] {
        auto Q = user_context->queries<cpp_examples::query_factory>();
        return named_products(Q.products(), input_name);
    }();
    input_name = "second";
    auto executable = std::move(processed)
                          .comment("Read helper-processed products")
                          .purpose("Verify composed request ownership");
    const auto& processed_rows =
        cpp_examples::require_value(executable.executeForList(*user_context).wait());
    check(processed_rows.len() == 1 && processed_rows[0].name() == "first",
          "returned requests own text and survive the local factory and function wrappers");
    auto r = user_context->memory_resource();
    const auto& list = cpp_examples::require_value(query.executeForList(*user_context).wait());
    check(list.len() == 2 && list[0].name() == "first", "list order and size");
    auto limited = query;
    limited.offset(1).limit(1).selectRelation("deliberatelyUnknownRelation");
    check(
        cpp_examples::require_value(limited.executeForCount(*user_context).wait()) == 2,
        "count clears projection, ordering, slicing and relation selection before scalar execution");
    check(limited.query().slice == Slice{1, 1} && limited.query().relations.size() == 1,
          "count executes an independent snapshot without modifying a reusable description");
    check(cpp_examples::require_value(query.executeForExists(*user_context).wait()) &&
              !cpp_examples::require_value(Query(*user_context)
                                               .offset(9)
                                               .comment("inspect absent offset")
                                               .purpose("qualify existence")
                                               .executeForExists(*user_context)
                                               .wait()),
          "exists retains source offset and tests compact row presence");
    const auto& page =
        cpp_examples::require_value(query.executeForPage(*user_context, 1, 1).wait());
    check(page.len() == 1 && page[0].name() == "second" && page.total_count == 2,
          "page combines unsliced count and typed destination-owned list execution");
    const auto& zero_page =
        cpp_examples::require_value(query.executeForPage(*user_context, 0, 0).wait());
    check(zero_page.is_empty() && zero_page.total_count == 2,
          "zero-sized pages retain the actual unsliced count");
    const auto& first = cpp_examples::require_value(query.executeForFirst(*user_context).wait());
    check(first && first->name() == "first", "first preserves the accepted one semantics");
    check(list.memory_resource() == r && list[0].name().get_allocator().resource() == r,
          "results allocate in executing context");
    const auto& one = cpp_examples::require_value(query.executeForOne(*user_context).wait());
    check(one && one->name() == "first", "one returns first of multiple matches");
    auto independent = query;
    independent.whereEqual("name", "absent");
    check(cpp_examples::require_value(independent.executeForList(*user_context).wait()).len() ==
                  0 &&
              cpp_examples::require_value(query.executeForList(*user_context).wait()).len() == 2,
          "copying a mutable query creates independent Context-owned state");
    check(!query.query().slice, "one does not mutate reusable query");
    const auto& empty = cpp_examples::require_value(Query(*user_context)
                                                        .whereEqual("name", "absent")
                                                        .purpose("Missing product")
                                                        .comment("No match")
                                                        .executeForOne(*user_context)
                                                        .wait());
    check(!empty, "one returns none for zero matches");
    // The operation must own a complete destination copy before the builder dies.
    auto* operation = &product_request(*user_context).executeForList(*user_context);
    check(cpp_examples::require_value(operation->wait()).len() == 2,
          "operation survives query builder destruction");
    auto other_context = global.make_user_context();
    cpp_examples::setup(*other_context);
    rejects(
        [&] {
            copy.executeForList(*other_context);
        },
        "execution requires bound Context");
    auto destination_copy = copy.copyTo(*other_context);
    check(destination_copy.memory_resource() == other_context->memory_resource() &&
              cpp_examples::require_value(destination_copy.executeForList(*other_context).wait())
                      .len() == 0,
          "explicit destination copy executes in its new Context");
    auto foreign_child = Query(*other_context);
    rejects(
        [&] {
            nested_copy.selectRelation("product", foreign_child);
        },
        "cross-Context composition rejected");
    auto compatible_child = foreign_child.copyTo(*user_context);
    nested_copy.selectRelation("product", compatible_child);
    Query staged(*user_context);
    staged.comment("Base comment");
    auto purposed = staged.purpose("Execute a copy");
    staged.whereEqual("name", "absent");
    check(cpp_examples::require_value(purposed.executeForList(*user_context).wait()).len() == 2,
          "purpose on an lvalue takes an independent snapshot");
    auto moved = std::move(copy);
    rejects(
        [&] {
            copy.query();
        },
        "moved-from descriptions fail clearly");
    check(cpp_examples::require_value(moved.executeForList(*user_context).wait()).len() == 2,
          "moving retains description storage");
    auto missing_context = global.make_user_context();
    const auto& missing_result =
        product_request(*missing_context).executeForList(*missing_context).wait();
    check(!missing_result &&
              missing_result.error().kind() ==
                  DataServiceError<ExampleExecutor::Error>::Kind::Runtime &&
              missing_result.error().runtime_error().kind() == RuntimeError::Kind::Graph &&
              missing_result.error().runtime_error().message() ==
                  "missing entity data service for entity: Product",
          "missing setup returns the source-shaped Graph error through its operation");
    rejects(
        [&] {
            Query(*user_context).purpose("Intent").executeForOne(*user_context);
        },
        "missing comment rejected");
    rejects(
        [&] {
            Query(*user_context).comment("Comment").purpose("　\t").executeForStream(*user_context);
        },
        "Unicode blank intent rejected");
    auto& executor = user_context->get(executor_ref);
    auto* descriptor = executor.schema_provider.get_entity("Product");
    auto& compiled = user_context->make<CompiledQuery>(
        executor.dialect.compile_select(r, *descriptor, query.query()));
    auto& cursor = executor.transport.stream_sql(r, std::move(compiled), 1);
    check(cursor.wait_next() != nullptr, "hold SQLite lease");
    auto& pending = query.executeForOne(*user_context);
    check(!pending.poll(), "one forwards pending child state");
    pending.cancel();
    cursor.cancel();
    check(cpp_examples::require_value(query.executeForOne(*user_context).wait()).has_value(),
          "one cancellation releases work and permits reuse");
}

static void sqlite_stream_lifecycle() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    cpp_examples::setup(*user_context);
    seed(*user_context, "first");
    seed(*user_context, "second");
    auto r = user_context->memory_resource();
    auto query = product_request(*user_context).stream(1);
    const auto& handle = cpp_examples::require_value(query.executeForStream(*user_context).wait());
    auto& stream = handle.get();
    auto* first = stream.wait_next();
    check(first && *first && first->value().name() == "first", "first stream entity");
    check(first->value().memory_resource() == r, "stream entity belongs to context");
    auto* second = stream.wait_next();
    check(second && *second && second->value().name() == "second", "second stream entity");
    check(!stream.wait_next(), "stream reaches EOF");
    check(first->value().name() == "first", "delivered items remain valid after next and EOF");
    const auto& early_handle = cpp_examples::require_value(
        product_request(*user_context).stream(1).executeForStream(*user_context).wait());
    auto& early = early_handle.get();
    check(early.wait_next() != nullptr, "early cancellation begins cursor");
    early.cancel();
    check(cpp_examples::require_value(
              product_request(*user_context).executeForOne(*user_context).wait())
              .has_value(),
          "stream cancellation releases SQLite connection");
    auto relations = product_request(*user_context).selectRelation("unknown");
    check(!relations.executeForStream(*user_context).wait(),
          "unsupported stream shape fails closed");
    const auto& active = cpp_examples::require_value(
        product_request(*user_context).stream(1).executeForStream(*user_context).wait());
    check(active.get().wait_next() != nullptr, "active cursor at close");
    auto stream_identity =
        user_context->make_ref<TypedEntityQueryStream<Product, ExampleExecutor::Error>>(active);
    user_context->close();
    rejects(
        [&] {
            user_context->get(stream_identity);
        },
        "stream handle expires at close");
    user_context.reset();
    check(global.live_user_bytes() == 0, "closing joins cursors and reclaims context blocks");
}

// Exercise pending, empty chunks, item errors and decoder failures independently
// of SQLite's immediate native cursor implementation.
struct Chunks {
    using allocator_type = Allocator;
    using Item = Result<StreamChunk, DataServiceError<String>>;
    Memory resource;
    unsigned& closes;
    bool malformed, failure, pending = true, closed = false;
    unsigned index = 0;

    Chunks(std::allocator_arg_t, Allocator a, unsigned& closes, bool malformed, bool failure)
        : resource(a.resource()), closes(closes), malformed(malformed), failure(failure) {}

    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        if (pending) {
            pending = false;
            wake.notify();
            return {};
        }
        if (index == 2) {
            return std::optional<Item>{};
        }
        auto current = index++;
        if (current == 1 && failure) {
            return std::optional<Item>(Item::err(
                resource,
                DataServiceError<String>::executor(resource, String("cursor failure", resource))));
        }
        StreamChunk chunk(std::allocator_arg, Allocator(resource), current, current == 1);
        if (current == 1) {
            chunk.rows.emplace_back();
            chunk.rows.back().insert("id",
                                     malformed ? Value(resource, "invalid identity")
                                               : Value(resource, std::uint64_t(1)));
            chunk.rows.back().insert("version", Value(resource, std::int64_t(1)));
            chunk.rows.back().insert("name", Value(resource, "decoded"));
        }
        return std::optional<Item>(Item::ok(resource, std::move(chunk)));
    }

    void finish_and_wait() noexcept {
        close();
    }

    void cancel_and_wait() noexcept {
        close();
    }

    void close() noexcept {
        if (!closed) {
            closed = true;
            ++closes;
        }
    }
};

static void stream_mapping_errors() {
    for (unsigned mode = 0; mode < 3; ++mode) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        unsigned closes = 0;
        using ChunkResult = Result<StreamChunk, DataServiceError<String>>;
        auto& source = make_stream<ChunkResult, Chunks>(r, closes, mode == 1, mode == 2);
        EntityQueryStream<String> handle(
            std::allocator_arg, Allocator(r), source, detail::resource_arena(r));
        auto& mapped = make_stream<Result<Product, DataServiceError<String>>,
                                   detail::TypedEntityStreamItems<Product, String>>(r, handle);
        auto ticket = mapped.wake_signal().ticket();
        check(mapped.poll_next().kind ==
                  StreamPoll<Result<Product, DataServiceError<String>>>::Kind::Pending,
              "stream forwards pending and wake");
        check(mapped.wake_signal().ticket() != ticket,
              "source notifications wake the typed stream");
        auto* item = mapped.wait_next();
        check(item != nullptr, "empty chunk is skipped");
        if (mode == 0) {
            check(*item && item->value().name() == "decoded", "compact row decodes into entity");
        } else {
            check(!*item && item->error().kind() ==
                                (mode == 1 ? DataServiceError<String>::Kind::Entity
                                           : DataServiceError<String>::Kind::Executor),
                  "stream preserves decoder versus executor errors");
        }
        check(!mapped.wait_next() && closes == 1, "terminal errors and EOF close source once");
        user_context->close();
        check(closes == 1, "close does not repeat resource cleanup");
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        compact_list_trace_and_intent();
        construction_and_execution();
        sqlite_stream_lifecycle();
        stream_mapping_errors();
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Runtime query list, one and stream contracts passed\n";
}
