#include "../../examples/generated/include/cpp_examples.hpp"
#include <teaql/runtime/entity_query.hpp>
#include <iostream>
#include <optional>
#include <string>

using namespace teaql;
using Model = cpp_examples::Product;
using Executor = cpp_examples::DataServiceExecutor;
using Query = EntityQuery<Model, Executor>;
using Purposed = EntityQuery<Model, Executor, true>;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& action, const char* message) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, message);
}

static auto customized(cpp_examples::ProductRequest<> request, std::string_view name) {
    return std::move(request).withNameIs(name);
}

class CountingResource final : public std::pmr::memory_resource {
public:
    std::size_t live_bytes = 0, allocations = 0, deallocations = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto* value = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        live_bytes += bytes;
        ++allocations;
        return value;
    }

    void do_deallocate(void* value, std::size_t bytes, std::size_t alignment) override {
        live_bytes -= bytes;
        ++deallocations;
        std::pmr::new_delete_resource()->deallocate(value, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void unused_queries_and_closed_wrappers() {
    CountingResource upstream;
    std::optional<Query> surviving_query;
    std::optional<cpp_examples::query_factory> surviving_factory;
    {
        GlobalContext global(&upstream);
        {
            auto user_context = global.make_user_context();
            check(user_context->allocation_stats().issued_objects == 0,
                  "no managed query roots at bootstrap");
            surviving_factory.emplace(user_context->queries<cpp_examples::query_factory>());
            check(user_context->allocation_stats().issued_objects == 0,
                  "factory is a non-owning value");
            surviving_query.emplace(*user_context);
            check(user_context->allocation_stats().issued_objects == 1,
                  "one query root consumes one identity");
            surviving_query->comment(std::string(4096, 'c'));
            surviving_query->whereEqual("name", std::string(8192, 'n'));
            check(user_context->allocation_stats().issued_objects == 1,
                  "strings consume payload bytes, not object identities");
            {
                auto Q = user_context->queries<cpp_examples::query_factory>();
                auto temporary = customized(Q.products(), std::string("temporary owned text"));
                auto copy = temporary;
                copy.withNameIs("copy only");
                check(global.live_user_bytes() > 0, "unused query data is charged to UserContext");
            }
            check(user_context->allocation_stats().live_objects >= 3,
                  "wrapper destruction leaves registered query roots for close");
            user_context->close();
            check(global.live_user_bytes() == 0, "close releases every unused query block");
            rejects(
                [&] {
                    surviving_query->query();
                },
                "closed query rejected");
            rejects(
                [&] {
                    surviving_factory->products();
                },
                "closed factory rejected");
        }
        // User has actually been deleted; only Global diagnostics are still alive.
        rejects(
            [&] {
                surviving_query->comment("late");
            },
            "deleted Context rejected before arena access");
        rejects(
            [&] {
                auto copy = *surviving_query;
                (void)copy;
            },
            "copy of deleted binding rejected");
        rejects(
            [&] {
                surviving_factory->orders();
            },
            "factory access after actual deletion rejected");
        surviving_factory.reset();
        surviving_query.reset();
        check(global.live_user_bytes() == 0,
              "wrapper destruction does not touch freed User memory");
    }
    check(upstream.live_bytes == 0 && upstream.allocations == upstream.deallocations,
          "custom upstream receives all blocks back after Global destruction");
}

static void allocation_failures() {
    GlobalContext global;
    MemoryLimits limits;
    limits.max_objects = 3;
    auto user_context = global.make_user_context(limits);
    auto Q = user_context->queries<cpp_examples::query_factory>();
    Query request(*user_context);
    auto copy = request;
    auto staged = request.purpose("third query root");
    bool objects_failed = false;
    try {
        auto extra = Q.products();
        (void)extra;
    } catch (const MemoryLimitError& error) {
        objects_failed = error.kind() == MemoryLimitError::Kind::Objects;
    }
    check(objects_failed, "query roots enforce Context object limit");
    request.comment("still usable");
    bool copy_failed = false;
    try {
        copy = request;
    } catch (const MemoryLimitError&) {
        copy_failed = true;
    }
    check(copy_failed, "failed copy reports allocation limit");
    // The failed assignment must leave its prior independent state unchanged.
    auto other_context = global.make_user_context();
    auto preserved = copy.copyTo(*other_context);
    check(!preserved.query().comment_text,
          "failed assignment preserved its original independent state");
    user_context->close();

    MemoryLimits bytes;
    bytes.max_bytes = 64 * 1024;
    auto small_context = global.make_user_context(bytes, 1024);
    Query bounded(*small_context);
    std::string oversized(128 * 1024, 'x');
    bool bytes_failed = false;
    try {
        bounded.whereEqual("name", oversized);
    } catch (const MemoryLimitError& error) {
        bytes_failed = error.kind() == MemoryLimitError::Kind::Bytes;
    }
    check(bytes_failed && bounded.query().entity == "Product",
          "oversized query payload fails within Context byte budget");
    small_context->close();
}

static void copies_domains_and_global_retention() {
    GlobalContext global;
    auto source_context = global.make_user_context();
    auto destination = global.make_user_context();
    Query original(*source_context);
    original.whereEqual("name", std::string("source-owned text")).comment("owned source comment");
    auto copy = original.copyTo(*destination);
    check(copy.memory_resource() == destination->memory_resource() && copy.query().predicate &&
              copy.query().predicate->memory_resource() == destination->memory_resource(),
          "explicit copies propagate the destination allocator into predicates");
    {
        // Raw kernel Expr values still own allocator-aware members. Their C++
        // destructors must run before their source Context is deleted. Q wrappers
        // deliberately contain only checked references and have no such payloads.
        auto expression = Expr::eq(source_context->memory_resource(), "name", "foreign domain");
        rejects(
            [&] {
                copy.filter(expression);
            },
            "raw foreign expressions need explicit copies");
        auto local_expression = detail::clone_in(destination->memory_resource(), expression);
        copy.filter(local_expression);
    }
    auto retained = original.copyToGlobal(*source_context);
    check(retained.memory_resource() == global.memory_resource(),
          "explicit Global copy uses delegated Global domain");
    source_context->close();
    source_context.reset();
    check(copy.query().entity == "Product" && copy.query().comment_text == "owned source comment",
          "complete destination copy survives source deletion");
    rejects(
        [&] {
            retained.query();
        },
        "Global copy still requires a live access binding");
    auto rebound = retained.rebindGlobal(*destination);
    check(rebound.query().comment_text == "owned source comment" &&
              rebound.memory_resource() == global.memory_resource(),
          "explicit Global rebind survives original User deletion");
    cpp_examples::setup(*destination);
    auto& product = Query(*destination)
                        .comment("Construct destination fixture")
                        .purpose("Seed retained-query fixture")
                        .newEntity(*destination);
    product.updateName("source-owned text");
    cpp_examples::require_value(
        AuditedSaveExt<Model>::save(std::move(product).audit_as("Seed destination fixture"),
                                    *destination)
            .wait());
    check(cpp_examples::require_value(
              rebound.purpose("Execute retained query").executeForList(*destination).wait())
                  .len() == 1,
          "retained nested predicate remains executable after source deletion");
    rejects(
        [&] {
            original.rebindGlobal(*destination);
        },
        "User data cannot masquerade as Global retention");
    GlobalContext other_global;
    auto foreign = other_global.make_user_context();
    rejects(
        [&] {
            retained.rebindGlobal(*foreign);
        },
        "rebind rejects foreign application");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        unused_queries_and_closed_wrappers();
        allocation_failures();
        copies_domains_and_global_retention();
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Context-bound query ownership contracts passed\n";
}
