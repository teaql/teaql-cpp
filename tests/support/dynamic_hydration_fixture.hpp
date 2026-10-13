#pragma once
#include "../../examples/models.hpp"
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>
#include <array>

namespace dynamic_hydration_fixture {
using namespace teaql;

// Handwritten carrier fixture, preserving the frozen School geometry. Generation
// remains unchanged until the shared runtime hydration contracts are qualified.
struct School : example::Model<School> {
    using Base = example::Model<School>;
    static constexpr std::string_view ENTITY_NAME = "School";
    eval::LoadState load_state;

    School(std::allocator_arg_t, Allocator a)
        : Base(std::allocator_arg, a), load_state(eval::LoadState::not_loaded(a.resource())) {}

    School(std::allocator_arg_t, Allocator a, const School& other)
        : Base(std::allocator_arg, a, other), load_state(std::allocator_arg, a, other.load_state) {}

    School(std::allocator_arg_t, Allocator a, School&& other)
        : School(detail::move_to(a.resource(), std::move(other))) {}

    School(School&&) noexcept = default;

    School& operator=(School&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.table("school_data");
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("name", DataType::Text);
        descriptor.property("version", DataType::I64).version();
        return descriptor;
    }

    static Result<FieldLayout, String> create_field_layout(Memory r) {
        const std::array<FieldLayout::Index, 11> indexes{{{"id", 0},
                                                          {"platform", 1},
                                                          {"school_type", 2},
                                                          {"name", 3},
                                                          {"address", 4},
                                                          {"established_date", 5},
                                                          {"student_capacity", 6},
                                                          {"active", 7},
                                                          {"create_time", 8},
                                                          {"update_time", 9},
                                                          {"version", 10}}};
        const std::array<FieldLayout::Mapping, 11> mappings{
            {{"id", "id", "id"},
             {"platform", "platform_id", "platform"},
             {"school_type", "school_type_id", "school_type"},
             {"name", "name", "name"},
             {"address", "address", "address"},
             {"established_date", "established_date", "established_date"},
             {"student_capacity", "student_capacity", "student_capacity"},
             {"active", "active", "active"},
             {"create_time", "create_time", "create_time"},
             {"update_time", "update_time", "update_time"},
             {"version", "version", "version"}}};
        const std::array<std::string_view, 11> members{"id",
                                                       "platform",
                                                       "school_type",
                                                       "name",
                                                       "address",
                                                       "established_date",
                                                       "student_capacity",
                                                       "active",
                                                       "create_time",
                                                       "update_time",
                                                       "version"};
        const std::array<std::string_view, 2> relations{"platform", "school_type"};
        return FieldLayout::from_generated(
            r,
            ENTITY_NAME,
            "14b4bfbb4cda258779665e3bc38ab991040fe75793a416d29eddd06f8fa8f586",
            indexes,
            mappings,
            relations,
            members);
    }

    static const FieldLayout& field_layout(Memory r) {
        return detail::ContextArena::from_memory(r).field_layout<School>();
    }

    static Result<School, EntityError> from_compact_row(Memory r, CompactRow row) {
        School entity(std::allocator_arg, Allocator(r));
        auto id = example::decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return Result<School, EntityError>::err(r, std::move(id).error());
        }
        entity.id = id.value();
        auto name = example::decode_field<String>(r, row, ENTITY_NAME, "name");
        if (!name) {
            return Result<School, EntityError>::err(r, std::move(name).error());
        }
        entity.name = std::move(name).value();
        entity.load_state = row.indexed_load_state(
            detail::ContextArena::from_memory(r).loaded_snapshots(), field_layout(r));
        entity.root.set_original_compact_row(ENTITY_NAME, std::move(row));
        return Result<School, EntityError>::ok(r, std::move(entity));
    }

    MutationValues into_values() && {
        return scalar_values();
    }

    static constexpr bool supports_dynamic_field_load() {
        return true;
    }

    std::optional<LoadedSnapshot::Borrow> loaded_state_snapshot() const {
        auto* snapshot = load_state.indexed_snapshot();
        return snapshot ? std::optional(*snapshot) : std::nullopt;
    }

    Result<std::monostate, EntityError>
    install_loaded_dynamic_fields(const DynamicFieldValues& fields, LoadedSnapshot::Borrow state) {
        root.install_loaded_dynamic_fields(fields);
        load_state = eval::LoadState::indexed(memory_resource(), state);
        return Result<std::monostate, EntityError>::ok(memory_resource(), {});
    }

    const DynamicFieldValues* dynamic_field_values() const {
        return root.loaded_dynamic_fields();
    }

    bool has_pending_dynamic_mutations() const {
        return root.has_pending_dynamic_mutations();
    }

    bool is_field_loaded(std::string_view code) const {
        return load_state.is_loaded(code);
    }
};

using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;

inline Executor& setup(UserContext& user_context,
                       SqliteConnection** native_connection = nullptr,
                       bool with_provider = true) {
    auto r = user_context.memory_resource();
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    metadata.register_entity(School::entity_descriptor(r));
    auto& registry = user_context.make<InMemoryEntityRegistry>();
    registry.register_entity(School::ENTITY_NAME);
    user_context.set_metadata(user_context, metadata);
    user_context.set_entity_registry(user_context, registry);
    auto& connection = user_context.make_resource<SqliteConnection>();
    if (native_connection) {
        *native_connection = &connection;
    }
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    auto& dialect = user_context.make_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto& executor = user_context.make<Executor>(dialect, transport, metadata, state);
    user_context.insert_resource(user_context, executor);
    if (!with_provider) {
        return executor;
    }
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
    DynamicStorageRows rows(r);
    rows.try_emplace(7).first->second.emplace(String("title", r), Value(r, "extensions"));
    rows.at(7).emplace(String("count", r), Value(r));
    const std::array owners{InMemoryDynamicFieldsProvider::Owner{&definitions, &rows}};
    auto& provider = user_context.make<InMemoryDynamicFieldsProvider>(
        std::move(InMemoryDynamicFieldsProvider::from_owners(r, owners)).value());
    user_context.set_dynamic_fields_provider(user_context, provider);
    return executor;
}
}
