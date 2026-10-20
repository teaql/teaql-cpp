#include <teaql/core/query.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static DynamicPropertyDefinitions schema(Memory owner, std::string_view name, DataType type) {
    const std::array<DynamicPropertyDefinitions::Field, 1> fields{{{name, type}}};
    auto value = DynamicPropertyDefinitions::from_fields(owner, fields);
    check(bool(value), "Schema fixture must be valid");
    return std::move(value).value();
}

static void managed_sharing_and_replacement() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto owner = user_context->memory_resource();
    auto& first =
        user_context->make<DynamicPropertyDefinitions>(schema(owner, "_rank", DataType::I64));
    auto& equal =
        user_context->make<DynamicPropertyDefinitions>(schema(owner, "_rank", DataType::I64));
    auto& replacement =
        user_context->make<DynamicPropertyDefinitions>(schema(owner, "_rank", DataType::Text));
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    query.project("id").comment("retain existing comment").purpose("retain existing purpose");
    auto expected = query.clone(owner);
    auto issued = user_context->allocation_stats().issued_objects;
    check(&query.with_dynamic_property_definitions(first) == &query,
          "The schema builder remains fluent");
    check(query.dynamic_property_definitions.get() == &first,
          "Managed same-Context metadata keeps source schema identity");
    auto copied = query.clone(owner);
    expected.dynamic_property_definitions =
        detail::make_box<DynamicPropertyDefinitions>(owner, first);
    check(query == expected && copied == query &&
              copied.dynamic_property_definitions.get() == &first,
          "Only the schema changes; a same-Context clone shares the immutable payload");
    query.with_dynamic_property_definitions(replacement);
    check(copied.dynamic_property_definitions.get() == &first &&
              copied.dynamic_property_definitions->data_type("_rank") == DataType::I64,
          "Replacement must not mutate previous query clones");
    query.with_dynamic_property_definitions(equal);
    check(query == copied && query.dynamic_property_definitions.get() == &equal &&
              query.dynamic_property_definitions.get() != copied.dynamic_property_definitions.get(),
          "Equal independently supplied schemas keep distinct identities");
    query.with_dynamic_property_definitions(*query.dynamic_property_definitions);
    check(query.dynamic_property_definitions.get() == &equal &&
              user_context->allocation_stats().issued_objects == issued,
          "Managed rebinding and cloning must not consume new object identities");
}

struct SchemaHolder {
    using allocator_type = Allocator;
    std::uint64_t prefix = 0;
    DynamicPropertyDefinitions definitions;

    SchemaHolder(std::allocator_arg_t, Allocator a, const DynamicPropertyDefinitions& value)
        : definitions(std::allocator_arg, a, value) {}
};

static void temporary_and_nested_values() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto owner = user_context->memory_resource();
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    {
        auto temporary = schema(owner, "_temporary", DataType::Text);
        query.with_dynamic_property_definitions(temporary);
        check(query.dynamic_property_definitions.get() != &temporary,
              "An allocator-aware stack value is copied into managed storage");
    }
    check(query.dynamic_property_definitions->data_type("_temporary") == DataType::Text,
          "The query must not retain the destroyed stack value");
    auto holder = user_context->make_ref<SchemaHolder>(*query.dynamic_property_definitions);
    auto& nested = user_context->get(holder).definitions;
    query.with_dynamic_property_definitions(nested);
    check(query.dynamic_property_definitions.get() != &nested,
          "A managed parent's subobject requires its own metadata payload");
    user_context->destroy(holder);
    check(query.dynamic_property_definitions->data_type("_temporary") == DataType::Text,
          "Destroying a temporary metadata container must not invalidate the query");
    query.dynamic_property_definitions =
        detail::make_box<DynamicPropertyDefinitions>(owner, *query.dynamic_property_definitions);
    auto* owning = query.dynamic_property_definitions.get();
    auto copy = query.clone(owner);
    check(copy.dynamic_property_definitions.get() != owning,
          "A legacy owning box cannot lend storage beyond its wrapper lifetime");
    query.with_dynamic_property_definitions(*query.dynamic_property_definitions);
    check(query.dynamic_property_definitions.get() != owning && query == copy,
          "Self-rebinding an owning box copies before releasing the old payload");
}

class FailingUpstream final : public std::pmr::memory_resource {
public:
    std::optional<std::size_t> fail_at;
    std::size_t attempts = 0;
    std::size_t bytes = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        if (fail_at == attempts++) {
            throw std::bad_alloc{};
        }
        auto* result = std::pmr::new_delete_resource()->allocate(size, alignment);
        bytes += size;
        return result;
    }

    void do_deallocate(void* value, std::size_t size, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(value, size, alignment);
        bytes -= size;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void explicit_retention() {
    FailingUpstream upstream;
    {
        GlobalContext global(&upstream);
        Ref<SelectQuery> retained;
        {
            auto source_user_context = global.make_user_context();
            auto destination_user_context = global.make_user_context();
            auto owner = source_user_context->memory_resource();
            SelectQuery* destination = nullptr;
            {
                String name(4096, 'n', owner);
                name.front() = '_';
                auto& definitions = source_user_context->make<DynamicPropertyDefinitions>(
                    schema(owner, name, DataType::Json));
                SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
                query.with_dynamic_property_definitions(definitions);
                destination = &destination_user_context->make<SelectQuery>(query);
                retained = source_user_context->make_global<SelectQuery>(query);
                check(destination->dynamic_property_definitions.get() != &definitions &&
                          destination->dynamic_property_definitions->memory_resource() ==
                              destination_user_context->memory_resource(),
                      "A destination Context receives its own metadata");
            }
            source_user_context.reset();
            const auto& fields = destination->dynamic_property_definitions->types();
            check(fields.begin()->first.size() == 4096 &&
                      fields.begin()->first.get_allocator().resource() ==
                          destination_user_context->memory_resource(),
                  "Explicit retention propagates nested strings and survives source deletion");
        }
        auto next_user_context = global.make_user_context();
        const auto& query = next_user_context->get(next_user_context->rebind_global(retained));
        check(query.dynamic_property_definitions->memory_resource() == global.memory_resource() &&
                  query.dynamic_property_definitions->types().begin()->first.size() == 4096,
              "The explicit Global copy remains independent after both User Contexts are deleted");
    }
    check(upstream.bytes == 0, "Closing all Contexts must return every arena block");
}

static void bounded_and_external_resources() {
    GlobalContext global;
    auto user_context = global.make_user_context(MemoryLimits{.max_objects = 1});
    auto owner = user_context->memory_resource();
    auto& initial =
        user_context->make<DynamicPropertyDefinitions>(schema(owner, "_rank", DataType::I64));
    auto& foreign = global.make<DynamicPropertyDefinitions>(
        schema(global.memory_resource(), "_rank", DataType::Text));
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    query.with_dynamic_property_definitions(initial);
    auto copy = query.clone(owner);
    bool rejected = false;
    try {
        query.with_dynamic_property_definitions(foreign);
    } catch (const MemoryLimitError& error) {
        rejected = error.kind() == MemoryLimitError::Kind::Objects;
    }
    check(rejected && query == copy && query.dynamic_property_definitions.get() == &initial,
          "Object-budget exhaustion leaves the previous managed binding intact");
    {
        std::pmr::monotonic_buffer_resource pool(std::pmr::new_delete_resource());
        auto value = schema(&pool, "_external", DataType::Json);
        SelectQuery plain(std::allocator_arg, Allocator(&pool), "Order");
        plain.with_dynamic_property_definitions(value);
        auto retained = plain.clone(&pool);
        check(plain == retained &&
                  plain.dynamic_property_definitions.get_deleter().resource == &pool &&
                  plain.dynamic_property_definitions.get() !=
                      retained.dynamic_property_definitions.get(),
              "Explicit non-Arena kernel resources retain ordinary scoped value ownership");
    }
}

static void allocation_failures() {
    unsigned failures = 0;
    bool completed = false;
    for (std::size_t offset = 0; offset < 12 && !completed; ++offset) {
        FailingUpstream upstream;
        {
            GlobalContext global(&upstream);
            auto user_context = global.make_user_context();
            auto owner = user_context->memory_resource();
            auto& initial = user_context->make<DynamicPropertyDefinitions>(
                schema(owner, "_rank", DataType::I64));
            String long_name(131072, 'n', global.memory_resource());
            long_name.front() = '_';
            auto& foreign = global.make<DynamicPropertyDefinitions>(
                schema(global.memory_resource(), long_name, DataType::Text));
            SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
            query.with_dynamic_property_definitions(initial);
            upstream.fail_at = upstream.attempts + offset;
            try {
                query.with_dynamic_property_definitions(foreign);
                completed = true;
            } catch (const std::bad_alloc&) {
                ++failures;
                check(query.dynamic_property_definitions.get() == &initial &&
                          query.dynamic_property_definitions->data_type("_rank") == DataType::I64,
                      "A failed destination allocation must preserve the existing binding");
                upstream.fail_at.reset();
                query.with_dynamic_property_definitions(foreign);
            }
            check(query.dynamic_property_definitions->memory_resource() == owner &&
                      query.dynamic_property_definitions->types().begin()->first.size() == 131072,
                  "Successful binding and retry must publish a complete destination copy");
        }
        check(upstream.bytes == 0, "Failed schema construction must not retain arena blocks");
    }
    check(failures > 0 && completed, "The fault sweep must cover failures and eventual success");
}

struct ShutdownSchemaProbe {
    const SelectQuery& source;
    Memory owner;
    bool copied = false;
    bool rejected_as_closed = false;
    Arena::Quiescence registration;

    void stop() noexcept {
        try {
            auto* arena = dynamic_cast<Arena*>(owner);
            auto issued = arena->stats().issued_objects;
            auto copy = source.clone(owner);
            copied = copy.dynamic_property_definitions->data_type("_rank") == DataType::I64 &&
                     copy.dynamic_property_definitions.get() !=
                         source.dynamic_property_definitions.get() &&
                     arena->stats().issued_objects == issued;
        } catch (const ReferenceError& error) {
            rejected_as_closed = error.kind() == ReferenceError::Kind::Closed;
        } catch (...) {
        }
    }
};

static void shutdown_diagnostics() {
    FailingUpstream upstream;
    {
        GlobalContext global(&upstream);
        MemoryLimits limits;
        limits.max_bytes = 16384;
        limits.max_objects = 2;
        limits.shutdown_reserve_bytes = 4096;
        UserContext user_context(global, limits, 128);
        auto owner = user_context.memory_resource();
        auto& definitions =
            user_context.make<DynamicPropertyDefinitions>(schema(owner, "_rank", DataType::I64));
        auto& query = user_context.make<SelectQuery>("Order");
        query.with_dynamic_property_definitions(definitions);
        ShutdownSchemaProbe probe{query, owner};
        probe.registration.attach(*dynamic_cast<Arena*>(owner), &probe, [](void* value) noexcept {
            static_cast<ShutdownSchemaProbe*>(value)->stop();
        });
        bool exhausted = false;
        try {
            for (;;) {
                (void)owner->allocate(512);
            }
        } catch (const MemoryLimitError& error) {
            exhausted = error.kind() == MemoryLimitError::Kind::Bytes;
        }
        auto attempts = upstream.attempts;
        upstream.fail_at = attempts;
        user_context.close();
        check(exhausted && probe.copied && !probe.rejected_as_closed &&
                  upstream.attempts == attempts && user_context.allocation_stats().live_bytes == 0,
              "Shutdown schema copies must use reserved payload storage after quota exhaustion");
        upstream.fail_at.reset();
    }
    check(upstream.bytes == 0, "Shutdown schema diagnostics must return every backing block");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        managed_sharing_and_replacement();
        temporary_and_nested_values();
        explicit_retention();
        bounded_and_external_resources();
        allocation_failures();
        shutdown_diagnostics();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
