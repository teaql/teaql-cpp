#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/core/detail/text.hpp>

namespace teaql {
namespace {
template <class T>
Result<T, DynamicFieldError> invalid(Memory r, std::string_view code, std::string_view field) {
    return Result<T, DynamicFieldError>::err(r, DynamicFieldError(r, code, field));
}

ArenaBorrow<DynamicFieldDefinitions> retain(Memory r, const DynamicFieldDefinitions& source) {
    auto& target = detail::async_arena(r);
    auto& origin = detail::async_arena(source.memory_resource());
    const auto& retained =
        origin.can_lend_to(target) ? source : target.make<DynamicFieldDefinitions>(source);
    return ArenaBorrow<DynamicFieldDefinitions>(std::allocator_arg, Allocator(r), retained);
}

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T result;

    Ready(std::allocator_arg_t, Allocator a, T result)
        : result(detail::move_in(a.resource(), std::move(result))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(result);
    }

    void cancel_and_wait() noexcept {}

    void finish_and_wait() noexcept {}
};

template <class T>
DynamicStorageFuture<T>& ready(Memory r, Result<T, DynamicFieldError> result) {
    return make_operation<Result<T, DynamicFieldError>, Ready<Result<T, DynamicFieldError>>>(
        r, std::move(result));
}

class DatabaseLoad {
public:
    using allocator_type = Allocator;
    using Output = Result<DynamicFieldBatch, DynamicFieldError>;

    DatabaseLoad(std::allocator_arg_t,
                 Allocator a,
                 const DynamicFieldStore& store,
                 std::string_view namespace_name,
                 const DynamicFieldDefinitions& definitions,
                 std::span<const std::uint64_t> ids,
                 const DynamicFieldSelection& selection,
                 const QueryIntent& intent)
        : store_(store), namespace_(namespace_name, a.resource()),
          definitions_(retain(a.resource(), definitions)),
          ids_(ids.begin(), ids.end(), a.resource()), selection_(std::allocator_arg, a, selection),
          intent_(std::allocator_arg, a, intent) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = namespace_.get_allocator().resource();
        if (!pending_) {
            pending_ =
                &store_.load_values(r, namespace_, definitions_.get(), ids_, selection_, intent_);
            if (pending_->memory_resource() != r) {
                pending_->cancel();
                pending_ = nullptr;
                throw std::invalid_argument("Dynamic storage operation belongs to another Context");
            }
            forward_.attach(pending_->wake_signal(), wake);
        }
        auto* result = pending_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return Output::err(r, detail::clone_in(r, result->error()));
        }
        auto identity = store_.source_identity();
        if (!identity) {
            return invalid<DynamicFieldBatch>(
                r, "DYNAMIC_FIELD_STORAGE_IDENTITY_REQUIRED", definitions_.get().owner_type());
        }
        auto bound = definitions_.get().with_storage_binding_shared(r, namespace_, *identity);
        return Output::ok(
            r, DynamicFieldBatch(std::allocator_arg, Allocator(r), bound.get(), result->value()));
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (pending_) {
            pending_->cancel();
            pending_ = nullptr;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

private:
    const DynamicFieldStore& store_;
    String namespace_;
    ArenaBorrow<DynamicFieldDefinitions> definitions_;
    Vector<std::uint64_t> ids_;
    DynamicFieldSelection selection_;
    QueryIntent intent_;
    DynamicStorageFuture<DynamicStorageRows>* pending_ = nullptr;
    WakeSignal::Forward forward_;
};

template <class T>
class ContextDynamicOperation {
public:
    using allocator_type = Allocator;
    using Output = Result<T, DynamicFieldError>;

    ContextDynamicOperation(std::allocator_arg_t, Allocator a, const UserContext& user_context)
        : user_context_(user_context), owner_(a.resource()), ids_(a.resource()),
          selection_(DynamicFieldSelection::all(a.resource())) {}

    ContextDynamicOperation(std::allocator_arg_t,
                            Allocator a,
                            const UserContext& user_context,
                            std::string_view owner,
                            std::span<const std::uint64_t> ids,
                            const DynamicFieldSelection& selection,
                            const QueryIntent& intent)
        : user_context_(user_context), owner_(owner, a.resource()),
          ids_(ids.begin(), ids.end(), a.resource()), selection_(std::allocator_arg, a, selection),
          intent_(std::in_place, std::allocator_arg, a, intent) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = owner_.get_allocator().resource();
        if (!pending_) {
            auto* provider = user_context_.get_resource<DynamicFieldsProvider>();
            if (!provider) {
                if constexpr (std::same_as<T, std::monostate>) {
                    return Output::ok(r, {});
                } else {
                    return invalid<T>(r, "DYNAMIC_FIELD_PROVIDER_MISSING", owner_);
                }
            }
            if constexpr (std::same_as<T, std::monostate>) {
                pending_ = &provider->ensure_schema(user_context_);
            } else {
                pending_ =
                    &provider->load_values(user_context_, owner_, ids_, selection_, *intent_);
            }
            if (pending_->memory_resource() != r) {
                pending_->cancel();
                pending_ = nullptr;
                throw std::invalid_argument(
                    "Dynamic provider operation belongs to another Context");
            }
            forward_.attach(pending_->wake_signal(), wake);
        }
        auto* result = pending_->poll();
        if (!result) {
            return {};
        }
        return detail::clone_in(r, *result);
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (pending_) {
            pending_->cancel();
            pending_ = nullptr;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

private:
    const UserContext& user_context_;
    String owner_;
    Vector<std::uint64_t> ids_;
    DynamicFieldSelection selection_;
    std::optional<QueryIntent> intent_;
    DynamicStorageFuture<T>* pending_ = nullptr;
    WakeSignal::Forward forward_;
};
}

DynamicFieldBatch::DynamicFieldBatch(std::allocator_arg_t,
                                     Allocator a,
                                     const DynamicFieldDefinitions& definitions,
                                     const DynamicStorageRows& rows)
    : definitions(retain(a.resource(), definitions)), rows(rows, a.resource()) {}

DynamicFieldBatch::DynamicFieldBatch(std::allocator_arg_t,
                                     Allocator a,
                                     const DynamicFieldBatch& other)
    : DynamicFieldBatch(std::allocator_arg, a, other.definitions.get(), other.rows) {}

Result<Vector<DynamicFieldValues>, DynamicFieldError>
DynamicFieldBatch::into_values(std::string_view owner_type,
                               std::span<const std::uint64_t> ids,
                               const DynamicFieldSelection& selection) && {
    auto r = memory_resource();
    if (definitions.get().owner_type() != owner_type) {
        return invalid<Vector<DynamicFieldValues>>(r, "DYNAMIC_FIELD_OWNER_MISMATCH", owner_type);
    }
    auto checked = selection.validate(definitions.get());
    if (!checked) {
        return Result<Vector<DynamicFieldValues>, DynamicFieldError>::err(
            r, std::move(checked).error());
    }
    std::pmr::map<std::uint64_t, std::size_t> occurrences(r);
    for (auto id : ids) {
        ++occurrences[id];
    }
    for (const auto& [id, values] : rows) {
        if (!occurrences.contains(id)) {
            return invalid<Vector<DynamicFieldValues>>(
                r, "DYNAMIC_FIELD_UNREQUESTED_OWNER", owner_type);
        }
    }
    Vector<DynamicFieldValues::Values> payloads(r);
    payloads.reserve(ids.size());
    for (auto id : ids) {
        auto found = rows.find(id);
        if (found == rows.end()) {
            return invalid<Vector<DynamicFieldValues>>(
                r, "DYNAMIC_FIELD_BATCH_OMITTED_OWNER", owner_type);
        }
        for (const auto& [code, value] : found->second) {
            if (!selection.contains(code)) {
                return invalid<Vector<DynamicFieldValues>>(
                    r, "DYNAMIC_FIELD_UNSELECTED_VALUE", owner_type);
            }
        }
        if (--occurrences[id] == 0) {
            payloads.emplace_back(std::move(found->second));
            rows.erase(found);
        } else {
            payloads.emplace_back(found->second);
        }
    }
    return DynamicFieldValues::from_batch(r, definitions.get(), payloads);
}

Result<DynamicFieldStreamPlan, DynamicFieldError>
DynamicFieldsProvider::prepare_stream_read(const UserContext& user_context,
                                           const DynamicFieldStore&,
                                           std::string_view owner,
                                           const DynamicFieldSelection&,
                                           const QueryIntent&) const {
    return invalid<DynamicFieldStreamPlan>(
        user_context.memory_resource(), "DYNAMIC_FIELD_STREAM_UNSUPPORTED", owner);
}

Result<DynamicFieldWrite, DynamicFieldError>
DynamicFieldsProvider::prepare_graph_write(const UserContext& user_context,
                                           const DynamicFieldStore&,
                                           const DynamicFieldDefinitions&,
                                           std::uint64_t,
                                           const DynamicFieldWrite::Changes&) const {
    return invalid<DynamicFieldWrite>(
        user_context.memory_resource(), "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED", "save");
}

DynamicStorageFuture<std::monostate>&
DynamicFieldsProvider::ensure_schema(const UserContext& user_context) const {
    auto r = user_context.memory_resource();
    return ready(r, Result<std::monostate, DynamicFieldError>::ok(r, {}));
}

namespace detail {
DatabaseDynamicProvider::DatabaseDynamicProvider(
    std::allocator_arg_t,
    Allocator a,
    std::string_view namespace_name,
    std::span<const DynamicFieldDefinitions* const> definitions,
    Resolver resolve)
    : namespace_(namespace_name, a.resource()), definitions_(a.resource()), resolve_(resolve) {
    for (auto* definition : definitions) {
        definitions_.emplace(String(definition->owner_type(), a.resource()),
                             retain(a.resource(), *definition));
    }
}

DatabaseDynamicProvider::DatabaseDynamicProvider(std::allocator_arg_t,
                                                 Allocator a,
                                                 const DatabaseDynamicProvider& other)
    : namespace_(other.namespace_, a.resource()), definitions_(a.resource()),
      resolve_(other.resolve_) {
    for (const auto& [owner, definition] : other.definitions_) {
        definitions_.emplace(String(owner, a.resource()), retain(a.resource(), definition.get()));
    }
}

Result<std::monostate, DynamicFieldError> DatabaseDynamicProvider::validate_configuration(
    Memory r,
    std::string_view namespace_name,
    std::span<const DynamicFieldDefinitions* const> definitions) {
    auto trimmed = namespace_name;
    trim(trimmed);
    if (trimmed.empty()) {
        return invalid<std::monostate>(r, "DYNAMIC_FIELD_NAMESPACE_REQUIRED", "namespace");
    }
    std::pmr::set<std::string_view, std::less<>> owners(r);
    for (auto* definition : definitions) {
        if (!definition) {
            throw std::invalid_argument("Dynamic provider definitions must not be null");
        }
        if (!owners.insert(definition->owner_type()).second) {
            return invalid<std::monostate>(
                r, "DYNAMIC_FIELD_DUPLICATE_OWNER", definition->owner_type());
        }
    }
    return Result<std::monostate, DynamicFieldError>::ok(r, {});
}

Result<ArenaBorrow<DynamicFieldDefinitions>, DynamicFieldError>
DatabaseDynamicProvider::definitions_for_context(const UserContext& user_context,
                                                 std::string_view owner) const {
    auto r = user_context.memory_resource();
    auto found = definitions_.find(owner);
    if (found == definitions_.end()) {
        return invalid<ArenaBorrow<DynamicFieldDefinitions>>(
            r, "DYNAMIC_FIELD_DEFINITIONS_MISSING", owner);
    }
    auto store = resolve_(user_context, owner);
    if (!store) {
        return Result<ArenaBorrow<DynamicFieldDefinitions>, DynamicFieldError>::err(
            r, std::move(store).error());
    }
    auto identity = store.value()->source_identity();
    if (!identity) {
        return invalid<ArenaBorrow<DynamicFieldDefinitions>>(
            r, "DYNAMIC_FIELD_STORAGE_IDENTITY_REQUIRED", owner);
    }
    return Result<ArenaBorrow<DynamicFieldDefinitions>, DynamicFieldError>::ok(
        r, found->second.get().with_storage_binding_shared(r, namespace_, *identity));
}

Result<DynamicFieldStreamPlan, DynamicFieldError>
DatabaseDynamicProvider::prepare_stream_read(const UserContext& user_context,
                                             const DynamicFieldStore& store,
                                             std::string_view owner,
                                             const DynamicFieldSelection& selection,
                                             const QueryIntent& intent) const {
    auto r = user_context.memory_resource();
    auto definitions = definitions_for_context(user_context, owner);
    if (!definitions) {
        return Result<DynamicFieldStreamPlan, DynamicFieldError>::err(
            r, std::move(definitions).error());
    }
    auto ambient = resolve_(user_context, owner);
    if (!ambient) {
        return Result<DynamicFieldStreamPlan, DynamicFieldError>::err(r,
                                                                      std::move(ambient).error());
    }
    auto plan = DynamicFieldStreamPlan::create(
        r, namespace_, definitions.value().get(), selection, *ambient.value(), intent);
    if (!plan) {
        return plan;
    }
    auto checked = plan.value().validate_store(store);
    if (!checked) {
        return Result<DynamicFieldStreamPlan, DynamicFieldError>::err(r,
                                                                      std::move(checked).error());
    }
    return plan;
}

Result<DynamicFieldWrite, DynamicFieldError>
DatabaseDynamicProvider::prepare_graph_write(const UserContext& user_context,
                                             const DynamicFieldStore& store,
                                             const DynamicFieldDefinitions& expected,
                                             std::uint64_t owner_id,
                                             const DynamicFieldWrite::Changes& changes) const {
    auto r = user_context.memory_resource();
    auto owner = expected.owner_type();
    auto found = definitions_.find(owner);
    if (found == definitions_.end()) {
        return invalid<DynamicFieldWrite>(r, "DYNAMIC_FIELD_DEFINITIONS_MISSING", owner);
    }
    if (!found->second.get().same_schema(expected)) {
        return invalid<DynamicFieldWrite>(r, "DYNAMIC_FIELD_DEFINITION_MISMATCH", owner);
    }
    auto ambient = resolve_(user_context, owner);
    if (!ambient) {
        return Result<DynamicFieldWrite, DynamicFieldError>::err(r, std::move(ambient).error());
    }
    if (!store.is_transaction_bound() || ambient.value()->binding_key() != store.binding_key()) {
        return invalid<DynamicFieldWrite>(r, "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED", owner);
    }
    auto identity = ambient.value()->source_identity();
    if (!identity) {
        return invalid<DynamicFieldWrite>(r, "DYNAMIC_FIELD_STORAGE_IDENTITY_REQUIRED", owner);
    }
    if (!expected.storage_binding_matches(namespace_, *identity)) {
        return invalid<DynamicFieldWrite>(r, "DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH", owner);
    }
    return Result<DynamicFieldWrite, DynamicFieldError>::ok(
        r,
        DynamicFieldWrite(
            std::allocator_arg, Allocator(r), namespace_, expected, owner_id, changes));
}

DynamicStorageFuture<std::monostate>&
DatabaseDynamicProvider::ensure_schema(const UserContext& user_context) const {
    auto store = resolve_(user_context, "schema");
    if (!store) {
        return ready(user_context.memory_resource(),
                     Result<std::monostate, DynamicFieldError>::err(user_context.memory_resource(),
                                                                    std::move(store).error()));
    }
    return store.value()->ensure_schema(user_context.memory_resource());
}

DynamicStorageFuture<DynamicFieldBatch>&
DatabaseDynamicProvider::load_values(const UserContext& user_context,
                                     std::string_view owner,
                                     std::span<const std::uint64_t> ids,
                                     const DynamicFieldSelection& selection,
                                     const QueryIntent& intent) const {
    auto r = user_context.memory_resource();
    auto found = definitions_.find(owner);
    if (found == definitions_.end()) {
        return ready(r, invalid<DynamicFieldBatch>(r, "DYNAMIC_FIELD_DEFINITIONS_MISSING", owner));
    }
    auto checked = selection.validate(found->second.get());
    if (!checked) {
        return ready(
            r, Result<DynamicFieldBatch, DynamicFieldError>::err(r, std::move(checked).error()));
    }
    auto store = resolve_(user_context, owner);
    if (!store) {
        return ready(
            r, Result<DynamicFieldBatch, DynamicFieldError>::err(r, std::move(store).error()));
    }
    return make_operation<Result<DynamicFieldBatch, DynamicFieldError>, DatabaseLoad>(
        r, *store.value(), namespace_, found->second.get(), ids, selection, intent);
}

Value mutation_audit_value(Memory r, const DynamicFieldMutation& mutation, const Value* actual) {
    Value::Object details(r);
    details.emplace(String("operation", r), Value(r, mutation.is_delete() ? "DELETE" : "SET"));
    if (!mutation.is_delete()) {
        details.emplace(String("value", r), Value(r, actual ? *actual : *mutation.value()));
    }
    return Value(r, std::move(details));
}
}

Result<InMemoryDynamicFieldsProvider, DynamicFieldError>
InMemoryDynamicFieldsProvider::from_owners(Memory r, std::span<const Owner> owners) {
    InMemoryDynamicFieldsProvider provider(r);
    for (const auto& owner : owners) {
        if (!owner.definitions || !owner.rows) {
            throw std::invalid_argument("Dynamic fixture owner must not be null");
        }
        auto name = owner.definitions->owner_type();
        if (provider.definitions_.contains(name)) {
            return invalid<InMemoryDynamicFieldsProvider>(r, "DYNAMIC_FIELD_DUPLICATE_OWNER", name);
        }
        provider.definitions_.emplace(String(name, r), retain(r, *owner.definitions));
        auto& rows = provider.values_.try_emplace(String(name, r)).first->second;
        for (const auto& [id, values] : *owner.rows) {
            auto fields = DynamicFieldValues::from_values(r, *owner.definitions, values);
            if (!fields) {
                return Result<InMemoryDynamicFieldsProvider, DynamicFieldError>::err(
                    r, std::move(fields).error());
            }
            rows.emplace(id, values);
        }
    }
    return Result<InMemoryDynamicFieldsProvider, DynamicFieldError>::ok(r, std::move(provider));
}

InMemoryDynamicFieldsProvider::InMemoryDynamicFieldsProvider(
    std::allocator_arg_t, Allocator a, const InMemoryDynamicFieldsProvider& other)
    : definitions_(a.resource()), values_(other.values_, a.resource()) {
    for (const auto& [owner, definition] : other.definitions_) {
        definitions_.emplace(String(owner, a.resource()), retain(a.resource(), definition.get()));
    }
}

DynamicStorageFuture<DynamicFieldBatch>&
InMemoryDynamicFieldsProvider::load_values(const UserContext& user_context,
                                           std::string_view owner,
                                           std::span<const std::uint64_t> ids,
                                           const DynamicFieldSelection& selection,
                                           const QueryIntent&) const {
    auto r = user_context.memory_resource();
    auto found = definitions_.find(owner);
    if (found == definitions_.end()) {
        return ready(r, invalid<DynamicFieldBatch>(r, "DYNAMIC_FIELD_DEFINITIONS_MISSING", owner));
    }
    auto checked = selection.validate(found->second.get());
    if (!checked) {
        return ready(
            r, Result<DynamicFieldBatch, DynamicFieldError>::err(r, std::move(checked).error()));
    }
    auto stored = values_.find(owner);
    DynamicStorageRows rows(r);
    for (auto id : ids) {
        auto [entry, inserted] = rows.try_emplace(id);
        if (!inserted || stored == values_.end()) {
            continue;
        }
        auto payload = stored->second.find(id);
        if (payload == stored->second.end()) {
            continue;
        }
        for (const auto& [code, value] : payload->second) {
            if (selection.contains(code)) {
                entry->second.emplace(String(code, r), Value(r, value));
            }
        }
    }
    return ready(
        r,
        Result<DynamicFieldBatch, DynamicFieldError>::ok(
            r, DynamicFieldBatch(std::allocator_arg, Allocator(r), found->second.get(), rows)));
}

Result<DynamicFieldStreamPlan, DynamicFieldError>
UserContext::prepare_dynamic_field_stream(const DynamicFieldStore& store,
                                          std::string_view owner,
                                          const DynamicFieldSelection& selection,
                                          const QueryIntent& intent) const {
    auto* provider = get_resource<DynamicFieldsProvider>();
    if (!provider) {
        return invalid<DynamicFieldStreamPlan>(
            memory_resource(), "DYNAMIC_FIELD_PROVIDER_MISSING", owner);
    }
    return provider->prepare_stream_read(*this, store, owner, selection, intent);
}

Result<const DynamicFieldsProvider*, RuntimeError> UserContext::dynamic_graph_provider() const {
    auto* provider = get_resource<DynamicFieldsProvider>();
    auto r = memory_resource();
    if (!provider || !provider->supports_graph_save()) {
        return Result<const DynamicFieldsProvider*, RuntimeError>::err(
            r,
            RuntimeError::dynamic_field(
                r, DynamicFieldError(r, "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED", "save")));
    }
    return Result<const DynamicFieldsProvider*, RuntimeError>::ok(r, provider);
}

DynamicStorageFuture<DynamicFieldBatch>&
UserContext::load_dynamic_fields(std::string_view owner,
                                 std::span<const std::uint64_t> ids,
                                 const DynamicFieldSelection& selection,
                                 const QueryIntent& intent) const {
    return make_operation<Result<DynamicFieldBatch, DynamicFieldError>,
                          ContextDynamicOperation<DynamicFieldBatch>>(
        memory_resource(), *this, owner, ids, selection, intent);
}

DynamicStorageFuture<std::monostate>& UserContext::ensure_dynamic_fields_schema() const {
    return make_operation<Result<std::monostate, DynamicFieldError>,
                          ContextDynamicOperation<std::monostate>>(memory_resource(), *this);
}
}
