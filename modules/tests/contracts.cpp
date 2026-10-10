#include <array>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

import teaql_memory;
import teaql_async;
import teaql_core;
import teaql_runtime;

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
