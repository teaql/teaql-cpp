#pragma once
#include <teaql/runtime.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/sql/executor.hpp>
#include <array>

namespace sqlite_driver_benchmark {
using namespace teaql;

inline void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Handwritten counterpart of the original benchmark carrier, not generated SDK code.
struct Probe : EntityDefaults<Probe> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "DriverProbe";
    CompactRow row;
    std::optional<EntityRuntimeState> state;
    eval::LoadState load_state;
    std::uint64_t id;
    std::int64_t version;
    String name;
    std::optional<String> note;

    Probe(std::allocator_arg_t, Allocator a, CompactRow value)
        : row(std::allocator_arg, a, std::move(value)),
          load_state(row.indexed_load_state(
              detail::ContextArena::from_memory(a.resource()).loaded_snapshots(),
              field_layout(a.resource()))),
          id(row.get("id")->try_u64().value()), version(row.get("version")->try_i64().value()),
          name(row.get("name")->try_text().value(), a.resource()) {
        if (const auto* value = row.get("note"); value && !value->is_null()) {
            note.emplace(value->try_text().value(), a.resource());
        }
    }

    Probe(std::allocator_arg_t, Allocator a, const Probe& other)
        : row(std::allocator_arg, a, other.row), state(detail::clone_in(a.resource(), other.state)),
          load_state(std::allocator_arg, a, other.load_state), id(other.id), version(other.version),
          name(other.name, a.resource()), note(detail::clone_in(a.resource(), other.note)) {}

    Probe(std::allocator_arg_t, Allocator a, Probe&& other)
        : Probe(detail::move_to(a.resource(), std::move(other))) {}

    Probe(const Probe&) = delete;
    Probe(Probe&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.table("driver_probe");
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        descriptor.property("note", DataType::Text);
        return descriptor;
    }

    static Result<FieldLayout, String> create_field_layout(Memory r) {
        const std::array<FieldLayout::Index, 4> indexes{
            {{"id", 0}, {"version", 1}, {"name", 2}, {"note", 3}}};
        const std::array<FieldLayout::Mapping, 4> mappings{{{"id", "id", "id"},
                                                            {"version", "version", "version"},
                                                            {"name", "name", "name"},
                                                            {"note", "note", "note"}}};
        const std::array<std::string_view, 4> members{"id", "version", "name", "note"};
        return FieldLayout::from_generated(
            r, ENTITY_NAME, "driver-probe-v1", indexes, mappings, {}, members);
    }

    static const FieldLayout& field_layout(Memory r) {
        return detail::ContextArena::from_memory(r).field_layout<Probe>();
    }

    static Result<Probe, EntityError> from_compact_row(Memory r, CompactRow row) {
        return Result<Probe, EntityError>::ok(
            r, Probe(std::allocator_arg, Allocator(r), std::move(row)));
    }

    bool is_field_loaded(std::string_view field) const {
        return load_state.is_loaded(field);
    }

    std::optional<LoadedSnapshot::Borrow> loaded_state_snapshot() const {
        const auto* snapshot = load_state.indexed_snapshot();
        return snapshot ? std::optional(*snapshot) : std::nullopt;
    }

    std::optional<AnyRef> runtime_state_any() const& {
        return state ? std::optional<AnyRef>(std::in_place, *state) : std::nullopt;
    }

    std::optional<FieldSet> dirty_fields() const {
        if (!state) {
            return {};
        }
        EntityKey key(std::allocator_arg,
                      Allocator(memory_resource()),
                      ENTITY_NAME,
                      Value(memory_resource(), id));
        auto fields = state->changed_field_names(key);
        return fields.empty() ? std::optional<FieldSet>{}
                              : std::optional<FieldSet>(std::move(fields));
    }

    void on_loaded(AnyRef context) {
        if (auto* root = context.get<EntityRuntimeState>()) {
            state.emplace(std::allocator_arg, Allocator(memory_resource()), *root);
            state->set_original_compact_row(ENTITY_NAME, row);
        }
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields = row.into_map(memory_resource());
        return result;
    }
};

static_assert(Entity<Probe>);

struct RecordingTransport final : SqlTransport<MutationExecutorError> {
    using allocator_type = Allocator;
    SqliteMutationExecutor& inner;
    Vector<CompiledQuery> queries;

    RecordingTransport(std::allocator_arg_t, Allocator a, SqliteMutationExecutor& source)
        : inner(source), queries(a.resource()) {}

    DataServiceOperationResult<Rows, MutationExecutorError>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& query) override {
        queries.emplace_back(query);
        return inner.fetch_all_compact_sql(r, query);
    }

    DataServiceOperationResult<std::uint64_t, MutationExecutorError>&
    execute_sql(Memory r, const CompiledQuery& query) override {
        return inner.execute_sql(r, query);
    }
};

using Executor = SqlDataServiceExecutor<MutationExecutorError>;

inline InMemoryMetadataStore& install_metadata(UserContext& user_context) {
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    metadata.register_entity(Probe::entity_descriptor(user_context.memory_resource()));
    user_context.set_metadata(user_context, metadata);
    return metadata;
}

inline PurposedSelectQuery make_query(Memory r, bool full, std::size_t count) {
    SelectQuery query(std::allocator_arg, Allocator(r), Probe::ENTITY_NAME);
    if (full) {
        query.projects({"id", "version", "name", "note"});
    } else {
        query.projects({"id", "version", "name"});
    }
    query.filter(
        Expr::gt(r, "version", std::int64_t{0}).and_expr(Expr::begin_with(r, "name", "row-")));
    query.order_asc("id").limit(count).comment("what: matched typed load-state driver probe");
    return PurposedSelectQuery(
        std::allocator_arg, Allocator(r), query, "why: isolate governed runtime query overhead");
}
}
