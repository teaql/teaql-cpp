#include <array>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

import teaql_memory;
import teaql_async;
import teaql_core;
import teaql_runtime;
import teaql_provider_sqlite;

teaql::Timestamp header_timestamp(std::int64_t);
const std::type_info& header_value_type();
const std::type_info& reverse_import_value_type();
const std::type_info& mixed_import_value_type();
bool header_value_matches(const teaql::Value&, std::string_view);
const std::type_info& header_metadata_type();
std::size_t header_metadata_size();
teaql::Memory header_metadata_memory(const teaql::MetadataStore&);
void verify_module_lookup();

namespace {
struct ImportedDynamicStore final : teaql::DynamicFieldStore {
    bool is_transaction_bound() const override {
        return false;
    }

    std::uintptr_t binding_key() const override {
        return 1;
    }

    std::optional<std::uint64_t> source_identity() const override {
        return 7;
    }

    teaql::DynamicStorageFuture<std::monostate>& ensure_schema(teaql::Memory) const override {
        throw std::logic_error("unexpected I/O");
    }

    teaql::DynamicStorageFuture<teaql::DynamicStorageRows>&
    load_values(teaql::Memory,
                std::string_view,
                const teaql::DynamicFieldDefinitions&,
                std::span<const std::uint64_t>,
                const teaql::DynamicFieldSelection&,
                const teaql::QueryIntent&) const override {
        throw std::logic_error("unexpected I/O");
    }

    teaql::Result<std::monostate, teaql::DynamicFieldError>
    validate_writes(teaql::Memory r, std::span<const teaql::DynamicFieldWrite>) const override {
        return teaql::Result<std::monostate, teaql::DynamicFieldError>::ok(r, {});
    }

    teaql::DynamicStorageFuture<std::monostate>&
    apply_writes(teaql::Memory,
                 std::span<const teaql::DynamicFieldWrite>,
                 const teaql::MutationIntent&) const override {
        throw std::logic_error("unexpected I/O");
    }
};

struct ImportedBusinessIdDefinition {
    static constexpr std::string_view field_name = "business_id";
    static constexpr std::string_view type_name = "ImportedBusinessId";

    static teaql::BusinessIdDefinition definition(teaql::Memory resource) {
        return teaql::BusinessIdDefinition(
            std::allocator_arg, teaql::Allocator(resource), field_name, "IMP", "imports");
    }
};

struct ImportedEntityGeometry {
    static teaql::Result<teaql::FieldLayout, teaql::String> create_field_layout(teaql::Memory r) {
        const std::array indexes{teaql::FieldLayout::Index{"id", 0},
                                 teaql::FieldLayout::Index{"version", 1}};
        const std::array mappings{teaql::FieldLayout::Mapping{"id", "id", "id"},
                                  teaql::FieldLayout::Mapping{"version", "version", "version"}};
        const std::array<std::string_view, 2> members{"id", "version"};
        return teaql::FieldLayout::from_generated(
            r, "ImportedEntity", "v1", indexes, mappings, {}, members);
    }
};

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct ReadyInteger {
    bool stopped = false;

    std::optional<int> poll(teaql::WakeSignal&) {
        return 42;
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        stopped = true;
    }
};

struct ScopedRecord {
    using allocator_type = teaql::Allocator;
    teaql::String label;

    ScopedRecord(std::allocator_arg_t, allocator_type allocator, std::string_view text)
        : label(text, allocator.resource()) {}

    ScopedRecord(std::allocator_arg_t, allocator_type allocator, const ScopedRecord& other)
        : label(other.label, allocator.resource()) {}

    teaql::Memory memory_resource() const {
        return label.get_allocator().resource();
    }
};

struct ImportedEntity : teaql::EntityDefaults<ImportedEntity> {
    using allocator_type = teaql::Allocator;
    static constexpr std::string_view ENTITY_NAME = "ImportedEntity";
    teaql::String label;

    ImportedEntity(std::allocator_arg_t, allocator_type allocator, std::string_view text)
        : label(text, allocator.resource()) {}

    ImportedEntity(std::allocator_arg_t, allocator_type allocator, const ImportedEntity& other)
        : label(other.label, allocator.resource()) {}

    ImportedEntity(const ImportedEntity&) = delete;
    ImportedEntity(ImportedEntity&&) noexcept = default;

    teaql::Memory memory_resource() const {
        return label.get_allocator().resource();
    }

    static teaql::EntityDescriptor entity_descriptor(teaql::Memory owner) {
        teaql::EntityDescriptor descriptor(std::allocator_arg, allocator_type(owner), ENTITY_NAME);
        descriptor.property("name", teaql::DataType::Text);
        return descriptor;
    }

    static teaql::Result<ImportedEntity, teaql::EntityError>
    from_compact_row(teaql::Memory owner, teaql::CompactRow row) {
        auto model = std::make_obj_using_allocator<ImportedEntity>(allocator_type(owner), "");
        if (const auto* value = row.get("name")) {
            model.label = value->try_text().value_or("");
        }
        return teaql::Result<ImportedEntity, teaql::EntityError>::ok(owner, std::move(model));
    }

    teaql::MutationValues into_values() && {
        auto owner = memory_resource();
        teaql::MutationValues values(owner);
        values.fields.emplace(teaql::String("name", owner), teaql::Value(owner, label));
        return values;
    }
};

static_assert(teaql::Entity<ImportedEntity>);

void context_and_copy_contracts() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        std::pmr::memory_resource* previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    teaql::GlobalContext global;
    auto user = global.make_user_context();
    auto& metadata = user->make<teaql::detail::UserContextMetadata>(*user);
    check(typeid(metadata) == header_metadata_type(),
          "Relocated runtime metadata has identical header and module RTTI");
    check(sizeof(metadata) == header_metadata_size(),
          "Relocated runtime metadata preserves its header and module layout");
    check(header_metadata_memory(metadata) == user->memory_resource(),
          "Imported runtime metadata dispatches through the header SPI");
    auto reference =
        user->make_ref<teaql::Value>("a long imported string that exceeds inline storage");
    auto& value = user->get(reference);
    check(header_value_matches(value, "a long imported string that exceeds inline storage"),
          "Header and imported Value cross the translation-unit boundary");
    check(typeid(value) == header_value_type(), "Header and module RTTI identity");
    check(typeid(value) == reverse_import_value_type(), "Reversed imports preserve type identity");
    check(typeid(value) == mixed_import_value_type(), "Header-before-import type identity");
    auto retained = user->make_global<teaql::Value>(value);
    auto custom = user->make_ref<ScopedRecord>("custom downstream allocator-aware record");
    auto custom_global = user->make_global<ScopedRecord>(user->get(custom));
    auto& snapshot = user->make<teaql::EntitySnapshot>();
    snapshot.fields.emplace(teaql::String("name", user->memory_resource()),
                            teaql::Value(user->memory_resource(), "old name"));
    auto& mutation = user->make<teaql::UpdateCommand>("Customer", 1u);
    mutation.old_values.emplace(user->memory_resource(), snapshot);
    mutation.value("name", "new name").expected_version(4).audit_as("imported correction");
    check(mutation.expected_version_value == 4, "Imported mutation builder and defaults");
    check(std::get<teaql::String>(mutation.old_values->fields.find("name")->second.data) ==
              "old name",
          "Imported snapshot remains distinct from mutation values");
    check(mutation.trace_chain.front().comment.get_allocator().resource() ==
              user->memory_resource(),
          "Imported trace propagates its owner");
    auto& operation = teaql::make_operation<int, ReadyInteger>(*user);
    check(operation.poll() && *operation.poll() == 42,
          "Imported async factory instantiates downstream state");
    {
        auto& model = user->make<ImportedEntity>("imported entity name");
        auto builder =
            teaql::core::EntityGraph::from_entity(user->memory_resource(), std::move(model));
        builder.comment("imported entity graph");
        auto& graph = user->make<teaql::core::EntityGraph>(std::move(builder).build());
        check(graph.root.entity_type == ImportedEntity::ENTITY_NAME,
              "Imported entity concept instantiates the original graph builder");
        check(std::get<teaql::String>(graph.root.values.fields.find("name")->second.data) ==
                  "imported entity name",
              "Handwritten downstream mapping supplies graph mutation values");
        check(graph.root.comment && *graph.root.comment == "imported entity graph",
              "Imported graph comment retains original builder behavior");
    }
    user.reset();

    auto next = global.make_user_context();
    check(header_value_matches(next->get(next->rebind_global(retained)),
                               "a long imported string that exceeds inline storage"),
          "Explicit Global copy survives actual User destruction");
    check(next->get(next->rebind_global(custom_global)).label.get_allocator().resource() ==
              global.memory_resource(),
          "Downstream custom allocator copy remains in the Global domain");
    bool expired = false;
    try {
        (void)next->get(reference);
    } catch (const teaql::ReferenceError&) {
        expired = true;
    }
    check(expired, "Imported checked reference rejects a destroyed User");
}

void core_and_abi_contracts() {
    static_assert(sizeof(teaql::Ref<int>) == sizeof(std::uint64_t));
    static_assert(std::same_as<teaql::core::Value, teaql::Value>);
    static_assert(!std::is_convertible_v<teaql::GeneratedValues, teaql::MutationValues>);
    static_assert(!std::is_convertible_v<teaql::EntitySnapshot, teaql::MutationValues>);
    static_assert(std::same_as<teaql::core::FieldLayout, teaql::FieldLayout>);
    teaql::GlobalContext layout_global;
    auto layout_user_context = layout_global.make_user_context();
    const std::array layout_indexes{teaql::FieldLayout::Index{"id", 0},
                                    teaql::FieldLayout::Index{"version", 1}};
    const std::array layout_mappings{
        teaql::FieldLayout::Mapping{"id", "id", "db_id"},
        teaql::FieldLayout::Mapping{"version", "version", "db_version"}};
    const std::array<std::string_view, 2> layout_members{"id", "version"};
    auto layout = teaql::FieldLayout::from_generated(layout_user_context->memory_resource(),
                                                     "ModuleItem",
                                                     "v1",
                                                     layout_indexes,
                                                     layout_mappings,
                                                     {},
                                                     layout_members);
    check(layout && layout.value().index("db_version") == 1,
          "Imported FieldLayout uses the canonical core implementation");
    auto& managed_layout = layout_user_context->make<teaql::FieldLayout>(layout.value());
    auto& snapshot_pool = layout_user_context->make<teaql::SnapshotPool>();
    auto& field_state = layout_user_context->make<teaql::EntityRuntimeState>();
    auto field_r = layout_user_context->memory_resource();
    teaql::EntityKey field_key(std::allocator_arg,
                               teaql::Allocator(field_r),
                               "Item",
                               teaql::Value(field_r, std::uint64_t{7}));
    field_state.set(field_key, "quantity", teaql::Value(field_r, std::int64_t{0}));
    field_state.set_comment("request reason");
    field_state.set_entity_comment(field_key, "entity annotation");
    check(field_state.get_comment() == "request reason" &&
              field_state.get_entity_comment(field_key) == "entity annotation",
          "Imported entity annotations remain separate from the root request reason");
    auto field_eval = teaql::detail::EntityRuntimeAccess::eval_field<std::int64_t>(
        field_r, teaql::eval::LoadState::fully_loaded(field_r), "quantity", [&] {
            return teaql::detail::EntityRuntimeAccess::current_field(
                field_state, field_key, "quantity", std::int64_t{9});
        });
    check(field_eval.is_value() && field_eval.get() == 0,
          "Imported field helpers preserve explicit zero mutation values");
    teaql::CheckResults imported_results(field_r);
    auto imported_location = teaql::ObjectLocation::root(field_r);
    teaql::GeneratedCheckerLogic::requiredText("", "name", imported_location, imported_results);
    teaql::GeneratedFieldChecks::require_loaded(
        false, teaql::CheckObjectStatus::Update, "quantity", imported_location, imported_results);
    check(imported_results.size() == 2 && imported_results[1].rule == teaql::CheckRule::InvalidType,
          "Imported generated-checker rules use the canonical runtime helpers");
    auto rebound_field_state = field_state.with_loaded_view_from(field_state);
    check(!teaql::detail::EntityRuntimeAccess::original_values_for(rebound_field_state, field_key),
          "Imported loaded-view and keyed original APIs preserve absence");
    check(!teaql::EntityDefaults<ImportedEntity>{}.loaded_state_snapshot(),
          "Handwritten entity defaults do not invent an Indexed snapshot");

    struct ImportedRelationEntity {
        teaql::EntityRuntimeState root;

        teaql::Memory memory_resource() const {
            return root.memory_resource();
        }
    };

    auto null_relation = teaql::GeneratedRelationAccess::object<ImportedRelationEntity>(
        field_r, "target", nullptr, true);
    check(null_relation.is_null(), "Imported relation evaluation preserves known Null");
    auto missing_relation = field_state.relation_list<std::uint64_t>("Item", 7, "missing");
    check(missing_relation.state() == teaql::LoadedRelation::NotLoaded,
          "Imported relation handles preserve a checked missing view");
    using ImportedBusinessId = teaql::GeneratedBusinessId<ImportedBusinessIdDefinition>;
    auto business_id = ImportedBusinessId::create(field_r, "  IMP-value  ");
    check(business_id && business_id.value().asStr() == "IMP-value" &&
              std::hash<ImportedBusinessId>{}(business_id.value()) ==
                  std::hash<std::string_view>{}("IMP-value"),
          "Imported strong Business-ID values preserve creation and hashing");
    auto empty_business_id = teaql::GeneratedBusinessIdAccess::from_canonical<ImportedBusinessId>(
        field_r, std::string_view{});
    check(empty_business_id.asStr().empty(),
          "Imported framework getters preserve canonical empty storage separately from creation");
    auto query_intent = teaql::QueryIntent::create(field_r, "what", "why");
    auto missing_intent = teaql::MutationIntent::from_optional(field_r, std::nullopt);
    check(query_intent && query_intent.value().purpose() == "why" && !missing_intent &&
              missing_intent.error().code() == "REQUEST_COMMENT_REQUIRED",
          "Imported validated request intents preserve distinct query/mutation contracts");
    auto& request_query = layout_user_context->make<teaql::SelectQuery>("Imported");
    request_query.comment("owned reason").purpose("owned purpose");
    auto request_envelope = teaql::QueryRequest::from_query(field_r, request_query, false, false);
    check(request_envelope && request_envelope.value().comment() == "owned reason" &&
              request_envelope.value().purpose() == "owned purpose" &&
              request_envelope.value().execution_trace_chain().size() == 2,
          "Imported request envelopes preserve intent independently of capture flags");
    auto& imported_mutation = layout_user_context->make<teaql::InsertCommand>("Imported");
    auto mutation_envelope =
        teaql::MutationRequest::create(field_r, imported_mutation, "owned mutation reason");
    check(mutation_envelope && mutation_envelope.value().comment() == "owned mutation reason" &&
              mutation_envelope.value().execution_trace_chain().size() == 1 &&
              mutation_envelope.value().diagnostic_redactions() == nullptr,
          "Imported mutation envelopes require independent validated root intent");
    auto& staged_mutation =
        layout_user_context->make<teaql::data_service::MutationCommand>(imported_mutation);
    auto staged_request = staged_mutation.request(field_r, "command entry");
    check(staged_request && staged_request.value().comment() == "command entry",
          "Imported standalone mutation commands acquire intent at the checked request boundary");
    auto& imported_arena =
        teaql::detail::ContextArena::from_memory(layout_user_context->memory_resource());
    const auto& imported_geometry = imported_arena.field_layout<ImportedEntityGeometry>();
    check(imported_geometry.memory_resource() == layout_global.memory_resource() &&
              imported_arena.loaded_snapshots().memory_resource() ==
                  layout_user_context->memory_resource(),
          "Imported runtime owns Global type geometry and User snapshot pools separately");
    auto& imported_columns = layout_user_context->make<teaql::ColumnLayout>(
        std::initializer_list<std::string_view>{"db_id"});
    teaql::Vector<teaql::Value> imported_values(layout_user_context->memory_resource());
    imported_values.emplace_back(std::int64_t{7});
    teaql::CompactRow imported_row(std::allocator_arg,
                                   teaql::Allocator(layout_user_context->memory_resource()),
                                   imported_columns,
                                   std::move(imported_values));
    auto imported_row_state = imported_row.indexed_load_state(snapshot_pool, managed_layout);
    check(imported_row_state.is_loaded("id") && !imported_row_state.is_loaded("version"),
          "Imported rows decode availability through the canonical typed layout");
    auto snapshot =
        teaql::LoadedSnapshot::fully_loaded(layout_user_context->memory_resource(), managed_layout)
            .into_shared(snapshot_pool);
    auto state = teaql::eval::LoadState::indexed(layout_user_context->memory_resource(), snapshot);
    check(state.is_loaded("id") && state.is_loaded("version"), "Imported indexed availability");
    const std::array declarations{
        teaql::DynamicPropertyDefinitions::Field{"_count", teaql::DataType::U64}};
    auto definitions = teaql::DynamicPropertyDefinitions::from_fields(
        layout_user_context->memory_resource(), declarations);
    check(definitions && definitions.value().data_type("_count") == teaql::DataType::U64,
          "Imported readonly result schema");
    static_assert(std::same_as<teaql::dynamic_properties::DynamicPropertyDefinitions,
                               teaql::DynamicPropertyDefinitions>);
    const std::array dynamic_fields{
        teaql::DynamicFieldDefinitions::Field{"extension", teaql::DataType::Text}};
    auto durable_schema = teaql::DynamicFieldDefinitions::from_fields(
        field_r, managed_layout.entity(), "v1", dynamic_fields);
    auto& managed_schema = layout_user_context->make<teaql::DynamicFieldDefinitions>(
        std::move(durable_schema).value());
    teaql::Value::Object extension_values(field_r);
    extension_values.emplace(teaql::String("extension", field_r), teaql::Value(field_r, nullptr));
    auto durable_values =
        teaql::DynamicFieldValues::from_values(field_r, managed_schema, extension_values);
    auto& managed_values =
        layout_user_context->make<teaql::DynamicFieldValues>(std::move(durable_values).value());
    check(managed_values.field("extension").value().state() == teaql::DynamicFieldState::Null,
          "Imported durable field NULL remains a loaded value");
    auto extended_state =
        teaql::LoadedSnapshot::with_dynamic_fields(snapshot_pool, snapshot, managed_values);
    check(extended_state && extended_state.value().get().is_loaded("#extension") &&
              extended_state.value().get().is_loaded("id"),
          "Imported dynamic availability preserves fixed slots");
    imported_row.set_loaded_dynamic_fields(managed_values);
    check(imported_row.loaded_dynamic_fields() && !imported_row.contains_key("#extension"),
          "Imported row sidecars remain separate from native columns");
    auto taken_extensions = imported_row.take_loaded_dynamic_fields();
    check(taken_extensions && !imported_row.loaded_dynamic_fields(),
          "Imported row dynamic take protocol");
    field_state.install_loaded_dynamic_fields(managed_values);
    teaql::EntityKey extension_key(std::allocator_arg,
                                   teaql::Allocator(field_r),
                                   managed_layout.entity(),
                                   teaql::Value(field_r, std::uint64_t{7}));
    auto extension_update = field_state.update_dynamic_field(
        extension_key, "extension", teaql::Value(field_r, "changed"));
    auto extension_changes = field_state.current_change_set();
    const teaql::runtime::DynamicFieldChanges* dynamic_changes =
        extension_changes.dynamic_changes();
    check(extension_update && !extension_update.value() && dynamic_changes &&
              dynamic_changes->find(extension_key)
                      ->second.find("extension")
                      ->second.value()
                      ->try_text() == "changed",
          "Imported runtime separates dynamic ledger intent from native fields");
    ImportedDynamicStore imported_store;
    auto storage_intent =
        teaql::QueryIntent::create(field_r, "read extensions", "cursor hydration");
    auto storage_selection = teaql::DynamicFieldSelection::all(field_r);
    auto storage_plan = teaql::DynamicFieldStreamPlan::create(field_r,
                                                              "module-profile",
                                                              managed_schema,
                                                              storage_selection,
                                                              imported_store,
                                                              storage_intent.value());
    check(storage_plan && bool(storage_plan.value().validate_store(imported_store)),
          "Imported dynamic storage plan binds executor provenance");
    auto& sqlite_connection = layout_user_context->make_resource<teaql::SqliteConnection>();
    auto& sqlite_provider =
        layout_user_context->make<teaql::SqliteMutationExecutor>(sqlite_connection);
    check(sqlite_provider.dynamic_field_store() == &sqlite_provider &&
              bool(sqlite_provider.ensure_schema(field_r).wait()),
          "Imported SQLite provider exposes and initializes native dynamic storage");
    const std::array<const teaql::DynamicFieldDefinitions*, 1> imported_definitions{
        &managed_schema};
    using ImportedProvider = teaql::runtime::dynamic_fields::DatabaseDynamicFieldsProvider<
        teaql::SqliteMutationExecutor>;
    auto imported_configuration =
        ImportedProvider::create(field_r, "profile", imported_definitions);
    auto& imported_provider =
        layout_user_context->make<ImportedProvider>(std::move(imported_configuration).value());
    layout_user_context->insert_resource(*layout_user_context, sqlite_provider);
    layout_user_context->set_dynamic_fields_provider(*layout_user_context, imported_provider);
    check(bool(layout_user_context->ensure_dynamic_fields_schema().wait()) &&
              bool(imported_provider.definitions_for_context(*layout_user_context,
                                                             managed_schema.owner_type())),
          "Imported UserContext provider resolves registered SQLite metadata");
    static_assert(std::same_as<teaql::dynamic_fields::DynamicFieldStore,
                               teaql::data_service::dynamic_fields::DynamicFieldStore>);
    static_assert(
        std::same_as<teaql::core::dynamic_fields::DynamicFieldValues, teaql::DynamicFieldValues>);
    teaql::Vector<teaql::CompactRow> imported_rows(field_r);
    teaql::CompactRow imported_dynamic_row(std::allocator_arg, teaql::Allocator(field_r));
    imported_dynamic_row.insert("id", teaql::Value(field_r, std::uint64_t(7)));
    imported_dynamic_row.set_loaded_dynamic_fields(managed_values);
    imported_rows.emplace_back(std::move(imported_dynamic_row));
    auto imported_decode = teaql::runtime::decode_compact_rows_with_read_metadata<ImportedEntity>(
        field_r, std::move(imported_rows), field_state);
    check(!imported_decode && imported_decode.error().message ==
                                  "dynamic fields require a runtime-owned indexed entity carrier",
          "Imported central decoder validates dynamic carrier before ordinary decoding");
    auto committed_audit_error = teaql::RuntimeError::audit_after_commit(
        field_r, teaql::RuntimeError::event(field_r, "sink failed after commit"));
    check(committed_audit_error.kind() == teaql::RuntimeError::Kind::AuditAfterCommit &&
              committed_audit_error.audit_after_commit_error().message() ==
                  "sink failed after commit" &&
              committed_audit_error.to_string(field_r).starts_with("AUDIT_DELIVERY_AFTER_COMMIT:"),
          "Imported runtime preserves committed status and owns the original audit failure");
    auto timestamp = header_timestamp(42);
    std::unordered_map<teaql::Timestamp, int> indexed;
    indexed.emplace(timestamp, 7);
    check(indexed.at(teaql::Timestamp(42)) == 7,
          "Imported standard hash specialization and equality");
    auto date = teaql::Date::from_ymd(2024, 2, 29);
    check(date && date->epoch_days() == 19782,
          "Imported out-of-line date functions link the existing core archive");
    check(teaql::core::time::Timestamp(42) == timestamp, "Nested Rust core namespace alias");
}
}

int main() {
    verify_module_lookup();
    context_and_copy_contracts();
    core_and_abi_contracts();
    std::cout << "Imported Context, templates, ownership, core exports and ABI contracts pass\n";
}
