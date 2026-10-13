#include <teaql/data_service/dynamic_fields.hpp>
#include <teaql/core/detail/text.hpp>

namespace teaql {
namespace {
Arena& arena_for(Memory r) {
    auto* arena = dynamic_cast<Arena*>(r);
    if (!arena) {
        throw std::invalid_argument("Dynamic storage metadata requires a Context arena");
    }
    return *arena;
}

ArenaBorrow<DynamicFieldDefinitions> retain(Memory r, const DynamicFieldDefinitions& source) {
    auto& target = arena_for(r);
    const auto& copied = arena_for(source.memory_resource()).can_lend_to(target)
                             ? source
                             : target.make<DynamicFieldDefinitions>(source);
    return ArenaBorrow<DynamicFieldDefinitions>(std::allocator_arg, Allocator(r), copied);
}

template <class T>
Result<T, DynamicFieldError> error(Memory r, std::string_view code, std::string_view field) {
    return Result<T, DynamicFieldError>::err(r, DynamicFieldError(r, code, field));
}
}

DynamicFieldWrite::DynamicFieldWrite(std::allocator_arg_t,
                                     Allocator a,
                                     std::string_view namespace_name,
                                     const DynamicFieldDefinitions& schema,
                                     std::uint64_t owner_id,
                                     const Changes& changes)
    : namespace_name(namespace_name, a.resource()), definitions(retain(a.resource(), schema)),
      owner_id(owner_id), changes(changes, a.resource()) {}

DynamicFieldWrite::DynamicFieldWrite(std::allocator_arg_t,
                                     Allocator a,
                                     const DynamicFieldWrite& other)
    : DynamicFieldWrite(std::allocator_arg,
                        a,
                        other.namespace_name,
                        other.definitions.get(),
                        other.owner_id,
                        other.changes) {}

DynamicFieldStreamPlan::DynamicFieldStreamPlan(Memory r,
                                               std::string_view namespace_name,
                                               ArenaBorrow<DynamicFieldDefinitions> definitions,
                                               const DynamicFieldSelection& selection,
                                               std::uintptr_t binding,
                                               std::uint64_t identity,
                                               const QueryIntent& intent)
    : namespace_(namespace_name, r), definitions_(std::move(definitions)),
      selection_(std::allocator_arg, Allocator(r), selection), binding_key_(binding),
      source_identity_(identity), intent_(std::allocator_arg, Allocator(r), intent) {}

DynamicFieldStreamPlan::DynamicFieldStreamPlan(std::allocator_arg_t,
                                               Allocator a,
                                               const DynamicFieldStreamPlan& other)
    : DynamicFieldStreamPlan(a.resource(),
                             other.namespace_,
                             retain(a.resource(), other.definitions()),
                             other.selection_,
                             other.binding_key_,
                             other.source_identity_,
                             other.intent_) {}

Result<DynamicFieldStreamPlan, DynamicFieldError>
DynamicFieldStreamPlan::create(Memory r,
                               std::string_view namespace_name,
                               const DynamicFieldDefinitions& definitions,
                               const DynamicFieldSelection& selection,
                               const DynamicFieldStore& store,
                               const QueryIntent& intent) {
    using Output = Result<DynamicFieldStreamPlan, DynamicFieldError>;
    intent.require_valid();
    auto checked = selection.validate(definitions);
    if (!checked) {
        return Output::err(r, std::move(checked).error());
    }
    auto trimmed = namespace_name;
    detail::trim(trimmed);
    if (trimmed.empty()) {
        return error<DynamicFieldStreamPlan>(r, "DYNAMIC_FIELD_NAMESPACE_REQUIRED", "namespace");
    }
    auto identity = store.source_identity();
    if (!identity) {
        return error<DynamicFieldStreamPlan>(
            r, "DYNAMIC_FIELD_STORAGE_IDENTITY_REQUIRED", definitions.owner_type());
    }
    auto bound = definitions.with_storage_binding_shared(r, namespace_name, *identity);
    return Output::ok(
        r,
        DynamicFieldStreamPlan(
            r, namespace_name, bound, selection, store.binding_key(), *identity, intent));
}

Result<std::monostate, DynamicFieldError>
DynamicFieldStreamPlan::validate_store(const DynamicFieldStore& store) const {
    auto r = memory_resource();
    if (store.binding_key() != binding_key_ ||
        store.source_identity() != std::optional(source_identity_)) {
        return error<std::monostate>(
            r, "DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH", definitions().owner_type());
    }
    return Result<std::monostate, DynamicFieldError>::ok(r, {});
}

Result<Vector<std::uint64_t>, DynamicFieldError>
DynamicFieldStreamPlan::owner_ids(std::span<const CompactRow> rows) const {
    auto r = memory_resource();
    Vector<std::uint64_t> ids(r);
    for (const auto& row : rows) {
        const auto* value = row.get("id");
        auto id = value ? value->try_u64() : std::nullopt;
        if (!id || *id == 0) {
            return error<Vector<std::uint64_t>>(
                r, "DYNAMIC_FIELD_OWNER_ID_REQUIRED", definitions().owner_type());
        }
        ids.push_back(*id);
    }
    return Result<Vector<std::uint64_t>, DynamicFieldError>::ok(r, std::move(ids));
}

Result<std::monostate, DynamicFieldError>
DynamicFieldStreamPlan::merge(std::span<CompactRow> rows,
                              std::span<const std::uint64_t> ids,
                              const DynamicStorageRows& stored) const {
    auto r = memory_resource();
    using Output = Result<std::monostate, DynamicFieldError>;
    auto invalid = [&](std::string_view code) {
        return error<std::monostate>(r, code, definitions().owner_type());
    };
    if (rows.size() != ids.size()) {
        return invalid("DYNAMIC_FIELD_BATCH_OMITTED_OWNER");
    }
    auto actual = owner_ids(rows);
    if (!actual) {
        return Output::err(r, std::move(actual).error());
    }
    if (!std::ranges::equal(actual.value(), ids)) {
        return invalid("DYNAMIC_FIELD_OWNER_MISMATCH");
    }
    std::pmr::set<std::uint64_t> requested(ids.begin(), ids.end(), r);
    for (const auto& [id, values] : stored) {
        if (!requested.contains(id)) {
            return invalid("DYNAMIC_FIELD_UNREQUESTED_OWNER");
        }
    }
    Vector<Value::Object> payloads(r);
    for (auto id : ids) {
        auto found = stored.find(id);
        if (found == stored.end()) {
            return invalid("DYNAMIC_FIELD_BATCH_OMITTED_OWNER");
        }
        for (const auto& [code, value] : found->second) {
            if (!selection_.contains(code)) {
                return invalid("DYNAMIC_FIELD_UNSELECTED_VALUE");
            }
        }
        payloads.push_back(clone_record(r, found->second));
    }
    auto values = DynamicFieldValues::from_batch(r, definitions(), payloads);
    if (!values) {
        return Output::err(r, std::move(values).error());
    }
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    for (std::size_t i = 0; i < rows.size(); ++i) {
        auto merged = rows[i].merge_loaded_dynamic_fields(values.value()[i], shapes);
        if (!merged) {
            return Output::err(r, std::move(merged).error());
        }
    }
    return Output::ok(r, {});
}
}
