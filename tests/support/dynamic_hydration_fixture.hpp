#pragma once
#include "../../examples/models.hpp"
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/runtime/generated_dynamic_fields.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>
#include <array>

namespace dynamic_hydration_fixture {
using namespace teaql;

// Keep the frozen example layout and the exact three-slot source-test layout
// as separate carrier types sharing the same runtime protocol implementation.
template <bool SourceLayout>
struct SchoolCarrier : example::Model<SchoolCarrier<SourceLayout>> {
    using Base = example::Model<SchoolCarrier<SourceLayout>>;
    static constexpr std::string_view ENTITY_NAME = "School";
    eval::LoadState load_state;
    std::int64_t version_value = 0;

    SchoolCarrier(std::allocator_arg_t, Allocator a)
        : Base(std::allocator_arg, a), load_state(eval::LoadState::not_loaded(a.resource())) {}

    SchoolCarrier(std::allocator_arg_t, Allocator a, const SchoolCarrier& other)
        : Base(std::allocator_arg, a, other), load_state(std::allocator_arg, a, other.load_state),
          version_value(other.version_value) {}

    SchoolCarrier(std::allocator_arg_t, Allocator a, SchoolCarrier&& other)
        : SchoolCarrier(detail::move_to(a.resource(), std::move(other))) {}

    SchoolCarrier(SchoolCarrier&&) noexcept = default;

    SchoolCarrier& operator=(SchoolCarrier&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.table("school_data");
        descriptor.property("id", DataType::U64).id().not_null();
        if constexpr (SourceLayout) {
            descriptor.property("version", DataType::I64).version().not_null();
            descriptor.property("name", DataType::Text).not_null();
        } else {
            descriptor.property("name", DataType::Text);
            descriptor.property("version", DataType::I64).version();
        }
        return descriptor;
    }

    static Result<FieldLayout, String> create_field_layout(Memory r) {
        if constexpr (SourceLayout) {
            const std::array<FieldLayout::Index, 3> indexes{
                {{"id", 0}, {"version", 1}, {"name", 2}}};
            const std::array<FieldLayout::Mapping, 3> mappings{
                {{"id", "id", "id"}, {"version", "version", "version"}, {"name", "name", "name"}}};
            const std::array<std::string_view, 3> members{"id", "version", "name"};
            const std::array<std::string_view, 0> relations{};
            return FieldLayout::from_generated(
                r, ENTITY_NAME, "dynamic-school-sqlite-v1", indexes, mappings, relations, members);
        }
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
        return detail::ContextArena::from_memory(r).field_layout<SchoolCarrier>();
    }

    static Result<SchoolCarrier, EntityError> from_compact_row(Memory r, CompactRow row) {
        if constexpr (SourceLayout) {
            auto validated = row.validate_dynamic_properties();
            if (!validated) {
                return Result<SchoolCarrier, EntityError>::err(
                    r,
                    EntityError(std::allocator_arg,
                                Allocator(r),
                                ENTITY_NAME,
                                std::move(validated).error()));
            }
        }
        SchoolCarrier entity(std::allocator_arg, Allocator(r));
        auto id = example::decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return Result<SchoolCarrier, EntityError>::err(r, std::move(id).error());
        }
        entity.id = id.value();
        auto name = example::decode_field<String>(r, row, ENTITY_NAME, "name");
        if (!name) {
            return Result<SchoolCarrier, EntityError>::err(r, std::move(name).error());
        }
        entity.name = std::move(name).value();
        if constexpr (SourceLayout) {
            auto version = example::decode_field<std::int64_t>(r, row, ENTITY_NAME, "version");
            if (!version) {
                return Result<SchoolCarrier, EntityError>::err(r, std::move(version).error());
            }
            entity.version_value = version.value();
        }

        entity.load_state = row.indexed_load_state(
            detail::ContextArena::from_memory(r).loaded_snapshots(), field_layout(r));
        entity.root.set_original_compact_row(ENTITY_NAME, std::move(row));
        return Result<SchoolCarrier, EntityError>::ok(r, std::move(entity));
    }

    std::int64_t version() const
        requires(SourceLayout)
    {
        return version_value;
    }

    void updateNativeName(std::string_view value)
        requires(SourceLayout)
    {
        this->name = value;
        this->root.set(this->entity_key(), "name", Value(this->memory_resource(), this->name));
    }

    MutationValues into_values() && {
        auto values = this->scalar_values();
        if constexpr (SourceLayout) {
            auto r = this->memory_resource();
            for (std::string_view code : {"id", "name"}) {
                if (!load_state.is_loaded(code)) {
                    values.fields.erase(String(code, r));
                }
            }
            if (load_state.is_loaded("version")) {
                values.fields.emplace(String("version", r), Value(r, version_value));
            }
        }
        return values;
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
        if constexpr (SourceLayout) {
            return GeneratedDynamicFields::install(this->root,
                                                   load_state,
                                                   field_layout(this->memory_resource()),
                                                   fields,
                                                   state,
                                                   ENTITY_NAME);
        }
        this->root.install_loaded_dynamic_fields(fields);
        load_state = eval::LoadState::indexed(this->memory_resource(), state);
        return Result<std::monostate, EntityError>::ok(this->memory_resource(), {});
    }

    Result<std::monostate, DynamicFieldError> updateDynamicField(std::string_view code,
                                                                 const Value& value)
        requires(SourceLayout)
    {
        return GeneratedDynamicFields::update(this->root,
                                              load_state,
                                              field_layout(this->memory_resource()),
                                              this->entity_key(),
                                              code,
                                              value);
    }

    Result<std::monostate, DynamicFieldError> deleteDynamicField(std::string_view code)
        requires(SourceLayout)
    {
        return GeneratedDynamicFields::erase(this->root,
                                             load_state,
                                             field_layout(this->memory_resource()),
                                             this->entity_key(),
                                             code);
    }

    const DynamicFieldValues* dynamic_field_values() const {
        return this->root.loaded_dynamic_fields();
    }

    bool has_pending_dynamic_mutations() const {
        return this->root.has_pending_dynamic_mutations();
    }

    bool is_field_loaded(std::string_view code) const {
        return load_state.is_loaded(code);
    }

    void set_checker_loaded_fields(FieldSet fields) {
        auto r = this->memory_resource();
        Vector<std::string_view> names(r);
        for (const auto& field : fields) {
            names.push_back(field);
        }
        load_state = eval::LoadState::partial(r, names)
                         .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                                       field_layout(r))
                         .value();
    }
};

using School = SchoolCarrier<false>;
using SourceSchool = SchoolCarrier<true>;

using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;

inline Executor& setup(UserContext& user_context,
                       SqliteConnection** native_connection = nullptr,
                       bool with_provider = true,
                       bool source_layout = false) {
    auto r = user_context.memory_resource();
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    metadata.register_entity(source_layout ? SourceSchool::entity_descriptor(r)
                                           : School::entity_descriptor(r));
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
