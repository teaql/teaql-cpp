#pragma once
#include <teaql/runtime/detail/data_service/ledger_plan.hpp>
#include <teaql/runtime/detail/data_service/entity_graph_execution.hpp>
#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <teaql/runtime/detail/commit_audit.hpp>
#include <teaql/runtime/dynamic_fields.hpp>
#include <bit>
#include <limits>

namespace teaql::detail {
template <class E>
class LedgerExecutionOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<LedgerExecutionOutcome, Error>;
    using Signature = std::pair<String, String>;
    using Updates = std::pmr::map<Signature, Vector<EntityKey>>;
    enum class Phase { Start, Classify, Prepare, Delete, Insert, Update, Dynamic, Done };

    struct DynamicState {
        using allocator_type = Allocator;
        EntityChangeSet change_set;
        Vector<DynamicFieldWrite> writes;
        Vector<EntityKey> keys;
        ScopedBox<DynamicReadbacks> readbacks;
        std::optional<QueryIntent> intent;
        std::optional<MutationIntent> mutation_intent;
        ScopedBox<DynamicFieldSelection> selection;
        DynamicStorageFuture<std::monostate>* apply = nullptr;
        DynamicStorageFuture<DynamicStorageRows>* read = nullptr;
        bool applied = false;

        DynamicState(std::allocator_arg_t, Allocator a, const EntityChangeSet& changes)
            : change_set(std::allocator_arg, a, changes), writes(a.resource()), keys(a.resource()) {
        }
    };

public:
    using allocator_type = Allocator;

    LedgerExecutionOperation(std::allocator_arg_t,
                             Allocator a,
                             const EntityDataService<E>& service,
                             const EntityRuntimeState& root,
                             const LedgerObjectLocations& locations)
        : service_(std::allocator_arg, a, service), root_(std::allocator_arg, a, root),
          locations_(locations, a.resource()), deleted_(a.resource()), new_(a.resource()),
          changes_(a.resource()), keys_(a.resource()), deletes_(a.resource()),
          inserts_(a.resource()), updates_(a.resource()), prepared_(a.resource()),
          generated_(a.resource()), fallback_(a.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        if (phase_ == Phase::Start) {
            if (root_.has_pending_dynamic_mutations()) {
                auto provider = context.dynamic_graph_provider();
                if (!provider) {
                    return fail(std::move(provider).error());
                }
            }
            auto comment = root_.get_comment();
            auto bound = EntityDataServiceAccess::bind_mutation_intent(
                service_, comment ? std::optional<std::string_view>(*comment) : std::nullopt);
            if (!bound) {
                return fail(std::move(bound).error());
            }
            auto privacy = EntityDataServiceAccess::bind_ledger_privacy(service_, root_);
            if (!privacy) {
                return fail(std::move(privacy).error());
            }
            if (auto error = EntityRuntimeAccess::first_composition_error(root_)) {
                String message("generated entity graph attachment failed before save: ", r);
                message += error->to_string(r);
                return fail(RuntimeError::graph(r, message));
            }
            if (auto comment = root_.get_comment()) {
                fallback_.emplace_back(TraceKind::AuditReason, service_.entity(), *comment);
            }
            deleted_ = root_.deleted_keys();
            new_ = root_.new_keys();
            // Metadata callbacks must not change this execution's captured writes.
            auto change_set = root_.current_change_set();
            for (const auto& key : deleted_) {
                if (new_.contains(key)) {
                    continue;
                }
                auto descriptor = context.require_entity(key.entity);
                if (!descriptor) {
                    return fail(std::move(descriptor).error());
                }
                if (descriptor.value().get().version_property() &&
                    !root_.get_original_version(key)) {
                    return missing_version("delete", key);
                }
                deletes_.emplace_back(key);
            }
            for (const auto& [key, record] : change_set.changes()) {
                if (deleted_.contains(key)) {
                    continue;
                }
                EntityValues checked(std::allocator_arg, Allocator(r), record);
                checked.fields.try_emplace(String("id", r), key.id);
                changes_.emplace(clone_in(r, key), std::move(checked));
                keys_.emplace_back(key);
            }
            if (auto* changes = change_set.dynamic_changes()) {
                if (!changes->empty()) {
                    dynamic_ = make_box<DynamicState>(r, change_set);
                }
                for (const auto& [key, fields] : *changes) {
                    if (deleted_.contains(key) || changes_.contains(key)) {
                        continue;
                    }
                    EntityValues record(std::allocator_arg, Allocator(r));
                    record.fields.emplace(String("id", r), Value(r, key.id));
                    changes_.emplace(clone_in(r, key), std::move(record));
                    keys_.emplace_back(key);
                }
                std::sort(keys_.begin(), keys_.end());
            }
            phase_ = Phase::Classify;
        }
        while (phase_ == Phase::Classify && index_ < keys_.size()) {
            const auto& key = keys_[index_];
            bool fresh = new_.contains(key);
            if (!fresh) {
                if (!read_) {
                    auto descriptor = context.require_entity(key.entity);
                    if (!descriptor) {
                        return fail(std::move(descriptor).error());
                    }
                    descriptor_.emplace(std::allocator_arg, Allocator(r), descriptor.value());
                    auto* id = descriptor_->get().id_property();
                    if (!id) {
                        String message("entity ", r);
                        message += key.entity;
                        message += " has no id property";
                        return fail(RuntimeError::graph(r, message));
                    }
                    auto lineage = trace(key);
                    read_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                        service_, key.entity, id->name, key.id, lineage);
                }
                forwarding_.attach(read_->wake_signal(), wake);
                auto* result = read_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(read_, nullptr);
                if (!*result) {
                    return R::err(r, copy_or_take_error(*completed, *result));
                }
                // The descriptor may be a view into the managed metadata store.
                // Revalidate that owner before inspecting it after suspension.
                (void)context.entity(key.entity);
                fresh = !result->value();
                if (!fresh && descriptor_->get().version_property() &&
                    !root_.get_original_version(key)) {
                    return missing_version("update", key);
                }
                descriptor_.reset();
            }
            if (fresh) {
                inserts_.emplace_back(key);
            } else {
                String signature(r);
                bool first_field = true;
                for (const auto& [field, value] : changes_.at(key).fields) {
                    if (!first_field) {
                        signature += ',';
                    }
                    first_field = false;
                    signature += field;
                }
                Signature batch{String(key.entity, r), std::move(signature)};
                updates_[std::move(batch)].emplace_back(key);
            }
            ++index_;
        }
        if (phase_ == Phase::Classify) {
            phase_ = Phase::Prepare;
        }
        if (phase_ == Phase::Prepare) {
            auto ordered =
                EntityDataServiceAccess::order_new_ledger_keys(service_, inserts_, changes_);
            if (!ordered) {
                return fail(std::move(ordered).error());
            }
            {
                auto resources = EntityDataServiceAccess::bind_transaction_resources(service_);
                for (const auto& key : ordered.value()) {
                    if (graph_unassigned(&key.id)) {
                        auto id = context.next_id(key.entity);
                        if (!id) {
                            return fail(std::move(id).error());
                        }
                        generated_.insert_or_assign(clone_in(r, key), Value(r, id.value()));
                    }
                }
            }
            EntityRuntimeAccess::enrich_allocated_trace_scopes(root_, generated_);
            auto rebound = rebind_relations();
            if (!rebound) {
                return fail(std::move(rebound).error());
            }
            auto dynamic = prepare_dynamic();
            if (!dynamic) {
                return fail(std::move(dynamic).error());
            }
            std::size_t start = 0;
            while (start < ordered.value().size()) {
                auto entity = std::string_view(ordered.value()[start].entity);
                auto descriptor = context.require_entity(entity);
                if (!descriptor) {
                    return fail(std::move(descriptor).error());
                }
                const auto& info = descriptor.value().get();
                BatchInsertCommand command(std::allocator_arg, Allocator(r), info.name);
                do {
                    const auto& key = ordered.value()[start];
                    EntityValues record(std::allocator_arg, Allocator(r), changes_.at(key));
                    Value id(r, key.id);
                    if (auto generated = generated_.find(key); generated != generated_.end()) {
                        id = Value(r, generated->second);
                    }
                    (void)context.entity(key.entity);
                    (void)descriptor.value().get();
                    record.fields.insert_or_assign(String("id", r), std::move(id));
                    if (auto* version = info.version_property()) {
                        auto found = record.fields.find(version->name);
                        if (graph_unassigned(found == record.fields.end() ? nullptr
                                                                          : &found->second)) {
                            record.fields.insert_or_assign(String(version->name, r),
                                                           Value(r, std::int64_t(1)));
                        }
                    }
                    timestamps(record, info, true);
                    ObjectLocation location(std::allocator_arg, Allocator(r));
                    if (auto found = locations_.find(key); found != locations_.end()) {
                        location = clone_in(r, found->second);
                    }
                    auto required = ContextFixAccess::validate_required_create_payload(
                        context, entity, record, location);
                    if (!required) {
                        return fail(std::move(required).error());
                    }
                    command.batch_values.emplace_back(record.into_mutation_values(r));
                    command.trace_chains.emplace_back(trace(key));
                    ++start;
                } while (start < ordered.value().size() && ordered.value()[start].entity == entity);
                prepared_.emplace_back(std::move(command));
            }
            index_ = 0;
            phase_ = Phase::Delete;
        }
        while (phase_ == Phase::Delete && index_ < deletes_.size()) {
            if (!mutation_) {
                const auto& key = deletes_[index_];
                DeleteCommand command(std::allocator_arg, Allocator(r), key.entity, key.id);
                command.expected_version_value = root_.get_original_version(key);
                auto lineage = trace(key);
                command.trace_chain.swap(lineage);
                mutation_ = &EntityDataServiceAccess::delete_internal(service_, command);
            }
            auto completed = poll_mutation(wake);
            if (!completed) {
                return {};
            }
            if (!*completed) {
                return R::err(r, std::move(*completed).error());
            }
            ++index_;
        }
        if (phase_ == Phase::Delete) {
            index_ = 0;
            phase_ = Phase::Insert;
        }
        while (phase_ == Phase::Insert && index_ < prepared_.size()) {
            if (!mutation_) {
                mutation_ = &EntityDataServiceAccess::execute_prepared_batch_insert(
                    service_, prepared_[index_]);
            }
            auto completed = poll_mutation(wake);
            if (!completed) {
                return {};
            }
            if (!*completed) {
                return R::err(r, std::move(*completed).error());
            }
            ++index_;
        }
        if (phase_ == Phase::Insert) {
            update_ = updates_.begin();
            phase_ = Phase::Update;
        }
        while (phase_ == Phase::Update && update_ != updates_.end()) {
            if (!mutation_) {
                const auto& [signature, keys] = *update_;
                auto descriptor = context.require_entity(signature.first);
                if (!descriptor) {
                    return fail(std::move(descriptor).error());
                }
                const auto& info = descriptor.value().get();
                BatchUpdateCommand command(std::allocator_arg, Allocator(r), info.name);
                std::string_view fields = signature.second;
                for (;;) {
                    auto end = fields.find(',');
                    command.update_fields.emplace_back(fields.substr(0, end));
                    if (end == std::string_view::npos) {
                        break;
                    }
                    fields.remove_prefix(end + 1);
                }
                auto include = [&](std::string_view field) {
                    if (std::find(command.update_fields.begin(),
                                  command.update_fields.end(),
                                  field) == command.update_fields.end()) {
                        command.update_fields.emplace_back(field);
                    }
                };
                if (info.property_by_name("update_time")) {
                    include("update_time");
                }
                if (auto* version = info.version_property()) {
                    include(version->name);
                }
                for (const auto& key : keys) {
                    EntityValues record(std::allocator_arg, Allocator(r), changes_.at(key));
                    record.fields.insert_or_assign(String("id", r), Value(r, key.id));
                    auto original = root_.get_original_version(key);
                    if (auto* version = info.version_property()) {
#ifndef NDEBUG
                        if (original == std::numeric_limits<std::int64_t>::max()) {
                            throw std::overflow_error("attempt to add with overflow");
                        }
#endif
                        auto next = original ? std::bit_cast<std::int64_t>(
                                                   std::bit_cast<std::uint64_t>(*original) + 1)
                                             : 2;
                        record.fields.insert_or_assign(String(version->name, r), Value(r, next));
                    }
                    timestamps(record, info, false);
                    command.batch_values.emplace_back(record.into_mutation_values(r));
                    command.batch_ids.emplace_back(key.id);
                    command.batch_expected_versions.emplace_back(original);
                    command.batch_old_values.emplace_back(
                        EntityRuntimeAccess::original_values_for(root_, key));
                    command.trace_chains.emplace_back(trace(key));
                }
                mutation_ =
                    &EntityDataServiceAccess::execute_prepared_batch_update(service_, command);
            }
            auto completed = poll_mutation(wake);
            if (!completed) {
                return {};
            }
            if (!*completed) {
                return R::err(r, std::move(*completed).error());
            }
            ++update_;
        }
        if (phase_ == Phase::Update) {
            index_ = 0;
            phase_ = Phase::Dynamic;
        }
        if (phase_ == Phase::Dynamic && dynamic_ && !dynamic_->writes.empty()) {
            auto completed = persist_dynamic(wake);
            if (!completed) {
                return {};
            }
            if (!*completed) {
                return fail(std::move(*completed).error());
            }
        }
        phase_ = Phase::Done;
        LedgerExecutionOutcome outcome(std::allocator_arg, Allocator(r));
        outcome.generated_ids = std::move(generated_);
        if (dynamic_) {
            outcome.dynamic_readbacks = std::move(dynamic_->readbacks);
        }
        return R::ok(r, std::move(outcome));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        read_ = nullptr;
        mutation_ = nullptr;
        dynamic_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (read_) {
            read_->cancel();
            read_ = nullptr;
        }
        if (mutation_) {
            mutation_->cancel();
            mutation_ = nullptr;
        }
        if (dynamic_) {
            if (dynamic_->apply) {
                dynamic_->apply->cancel();
            }
            if (dynamic_->read) {
                dynamic_->read->cancel();
            }
            dynamic_.reset();
        }
    }

private:
    RuntimeHookResult rebind_relations() {
        auto r = service_.memory_resource();
        if (generated_.empty()) {
            return RuntimeHookResult::ok(r, {});
        }
        using Binding = std::pair<EntityKey, String>;
        std::pmr::map<Binding, Value> bindings(r);
        const auto& context = EntityDataServiceAccess::context(service_);
        for (const auto* descriptor : context.all_entities()) {
            for (const auto& relation : descriptor->relations) {
                std::string_view referencing, field, referenced;
                if (relation.many && relation.local_key == "id") {
                    referencing = relation.target_entity;
                    field = relation.foreign_key;
                    referenced = descriptor->name;
                } else if (!relation.many && relation.foreign_key == "id") {
                    referencing = descriptor->name;
                    field = relation.local_key;
                    referenced = relation.target_entity;
                } else {
                    continue;
                }
                for (const auto& [key, record] : changes_) {
                    if (key.entity != referencing) {
                        continue;
                    }
                    auto value = record.fields.find(field);
                    if (value == record.fields.end() || value->second.is_null()) {
                        continue;
                    }
                    EntityKey target(std::allocator_arg, Allocator(r), referenced, value->second);
                    auto assigned = generated_.find(target);
                    if (assigned == generated_.end()) {
                        continue;
                    }
                    Binding binding{clone_in(r, key), String(field, r)};
                    auto [current, inserted] =
                        bindings.try_emplace(std::move(binding), Value(r, assigned->second));
                    if (!inserted && current->second != assigned->second) {
                        String message("conflicting allocated relation identities for ", r);
                        message += key.entity;
                        message += '.';
                        message += field;
                        return RuntimeHookResult::err(r, RuntimeError::graph(r, message));
                    }
                }
            }
        }
        for (const auto& [binding, id] : bindings) {
            changes_.at(binding.first)
                .fields.insert_or_assign(String(binding.second, r), Value(r, id));
        }
        return RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult prepare_dynamic() {
        auto r = service_.memory_resource();
        if (!dynamic_) {
            return RuntimeHookResult::ok(r, {});
        }
        const auto& context = EntityDataServiceAccess::context(service_);
        if (!has_audit_scope(context)) {
            return dynamic_error("DYNAMIC_FIELD_AUDIT_SCOPE_REQUIRED", "save");
        }
        auto provider = context.dynamic_graph_provider();
        if (!provider) {
            return RuntimeHookResult::err(r, std::move(provider).error());
        }
        auto* store = EntityDataServiceAccess::dynamic_field_store(service_);
        if (!store) {
            return dynamic_error("DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED", "save");
        }
        for (const auto& [key, changes] : *dynamic_->change_set.dynamic_changes()) {
            if (deleted_.contains(key)) {
                continue;
            }
            auto* definitions = EntityRuntimeAccess::dynamic_definitions(dynamic_->change_set, key);
            if (!definitions) {
                return dynamic_error("DYNAMIC_FIELD_DEFINITIONS_MISSING", key.entity);
            }
            auto allocated = generated_.find(key);
            auto id = (allocated == generated_.end() ? key.id : allocated->second).try_u64();
            if (!id) {
                return dynamic_error("DYNAMIC_FIELD_OWNER_ID_REQUIRED", key.entity);
            }
            auto write =
                provider.value()->prepare_graph_write(context, *store, *definitions, *id, changes);
            if (!write) {
                return RuntimeHookResult::err(r, RuntimeError::dynamic_field(r, write.error()));
            }
            dynamic_->writes.emplace_back(std::move(write).value());
            dynamic_->keys.emplace_back(key);
        }
        auto validated = store->validate_writes(r, dynamic_->writes);
        if (!validated) {
            return RuntimeHookResult::err(r, RuntimeError::dynamic_field(r, validated.error()));
        }
        return RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult dynamic_error(std::string_view code, std::string_view field) {
        auto r = service_.memory_resource();
        return RuntimeHookResult::err(
            r, RuntimeError::dynamic_field(r, DynamicFieldError(r, code, field)));
    }

    template <class T>
    void validate_dynamic_future(DynamicStorageFuture<T>& future) {
        if (future.memory_resource() != service_.memory_resource()) {
            future.cancel();
            throw std::invalid_argument("Dynamic ledger operation belongs to a different Context");
        }
    }

    std::optional<RuntimeHookResult> persist_dynamic(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        auto* store = EntityDataServiceAccess::dynamic_field_store(service_);
        if (!store) {
            return dynamic_error("DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED", "save");
        }
        auto& state = *dynamic_;
        if (!state.applied) {
            if (!state.apply) {
                auto comment = root_.get_comment();
                auto intent = MutationIntent::from_optional(
                    r, comment ? std::optional<std::string_view>(*comment) : std::nullopt);
                if (!intent) {
                    return RuntimeHookResult::err(r,
                                                  RuntimeError::request_intent(r, intent.error()));
                }
                auto query = QueryIntent::create(
                    r, intent.value().comment(), "runtime: read authoritative graph extensions");
                if (!query) {
                    return RuntimeHookResult::err(r,
                                                  RuntimeError::request_intent(r, query.error()));
                }
                state.intent.emplace(std::move(query).value());
                state.mutation_intent.emplace(std::move(intent).value());
                state.apply = &store->apply_writes(r, state.writes, *state.mutation_intent);
                validate_dynamic_future(*state.apply);
            }
            forwarding_.attach(state.apply->wake_signal(), wake);
            auto* result = state.apply->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return RuntimeHookResult::err(r, RuntimeError::dynamic_field(r, result->error()));
            }
            forwarding_.reset();
            state.apply = nullptr;
            state.applied = true;
            state.readbacks = make_box<DynamicReadbacks>(r);
        }
        while (index_ < state.writes.size()) {
            const auto& write = state.writes[index_];
            const auto& key = state.keys[index_];
            if (!state.read) {
                std::pmr::set<String, std::less<>> codes(r);
                for (const auto& [code, mutation] : write.changes) {
                    codes.emplace(code);
                }
                if (key.entity == service_.entity()) {
                    if (auto* loaded = root_.loaded_dynamic_fields()) {
                        codes.insert(loaded->selected_codes().begin(),
                                     loaded->selected_codes().end());
                    }
                }
                Vector<DynamicFieldSelection::Field> fields(r);
                for (const auto& code : codes) {
                    auto type = write.definitions.get().data_type(code);
                    if (!type) {
                        return RuntimeHookResult::err(r,
                                                      RuntimeError::dynamic_field(r, type.error()));
                    }
                    fields.emplace_back(code, type.value());
                }
                auto selection = DynamicFieldSelection::fields(r, fields);
                if (!selection) {
                    return RuntimeHookResult::err(
                        r, RuntimeError::dynamic_field(r, selection.error()));
                }
                state.selection = make_box<DynamicFieldSelection>(r, std::move(selection).value());
                auto ids = std::span<const std::uint64_t>(&write.owner_id, 1);
                state.read = &store->load_values(r,
                                                 write.namespace_name,
                                                 write.definitions.get(),
                                                 ids,
                                                 *state.selection,
                                                 *state.intent);
                validate_dynamic_future(*state.read);
            }
            forwarding_.attach(state.read->wake_signal(), wake);
            auto* result = state.read->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return RuntimeHookResult::err(r, RuntimeError::dynamic_field(r, result->error()));
            }
            auto found = result->value().find(write.owner_id);
            if (result->value().size() != 1 || found == result->value().end()) {
                return dynamic_error("DYNAMIC_FIELD_READBACK_MISMATCH", key.entity);
            }
            const auto& values = found->second;
            for (const auto& [code, value] : values) {
                if (!state.selection->contains(code)) {
                    return dynamic_error("DYNAMIC_FIELD_UNSELECTED_VALUE", key.entity);
                }
            }
            for (const auto& [code, mutation] : write.changes) {
                if (!mutation.is_delete() != values.contains(code)) {
                    return dynamic_error("DYNAMIC_FIELD_READBACK_MISMATCH", code);
                }
            }
            auto fields = DynamicFieldValues::from_values(r, write.definitions.get(), values);
            if (!fields) {
                return RuntimeHookResult::err(r, RuntimeError::dynamic_field(r, fields.error()));
            }
            Vector<EntityPropertyChange> changes(r);
            auto* originals = EntityRuntimeAccess::dynamic_originals(state.change_set, key);
            for (const auto& [code, mutation] : write.changes) {
                std::optional<Value> original;
                if (originals) {
                    if (auto found = originals->find(code); found != originals->end()) {
                        original = clone_in(r, found->second);
                    }
                }
                auto actual = values.find(code);
                String field("#", r);
                field += code;
                changes.emplace_back(
                    field,
                    original,
                    std::optional<Value>(mutation_audit_value(
                        r, mutation, actual == values.end() ? nullptr : &actual->second)));
            }
            auto enriched = enrich_dynamic_audit(context, key.entity, write.owner_id, changes);
            if (!enriched) {
                return enriched;
            }
            state.readbacks->rows.emplace(clone_in(r, key), std::move(fields).value());
            forwarding_.reset();
            state.read = nullptr;
            state.selection.reset();
            ++index_;
        }
        return RuntimeHookResult::ok(r, {});
    }

    static void timestamps(EntityValues& record, const EntityDescriptor& descriptor, bool fresh) {
        auto r = record.memory_resource();
        auto now = Timestamp::now();
        for (auto name : {std::string_view("create_time"), std::string_view("update_time")}) {
            auto found = record.fields.find(name);
            if ((fresh || name == "update_time") && descriptor.property_by_name(name) &&
                graph_unassigned(found == record.fields.end() ? nullptr : &found->second)) {
                record.fields.insert_or_assign(String(name, r), Value(r, now));
            }
        }
    }

    TraceChain trace(const EntityKey& key) const {
        auto specific = root_.get_trace_chain(key);
        return specific.empty() ? TraceChain(fallback_, service_.memory_resource())
                                : std::move(specific);
    }

    R fail(RuntimeError error) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, std::move(error)));
    }

    R missing_version(std::string_view kind, const EntityKey& key) {
        auto r = service_.memory_resource();
        String message("cannot ", r);
        message += kind;
        message += ' ';
        message += key.entity;
        message += '(';
        message += debug_value(r, key.id);
        message += ") without its loaded original version; load the full entity before mutation";
        return fail(RuntimeError::graph(r, message));
    }

    std::optional<Result<std::uint64_t, Error>> poll_mutation(WakeSignal& wake) {
        forwarding_.attach(mutation_->wake_signal(), wake);
        auto* result = mutation_->poll();
        if (!result) {
            return {};
        }
        forwarding_.reset();
        auto* completed = std::exchange(mutation_, nullptr);
        return copy_or_take_completed(*completed, *result);
    }

    EntityDataService<E> service_;
    EntityRuntimeState root_;
    LedgerObjectLocations locations_;
    EntityRuntimeState::Keys deleted_, new_;
    LedgerChanges changes_;
    Vector<EntityKey> keys_, deletes_, inserts_;
    Updates updates_;
    Vector<BatchInsertCommand> prepared_;
    LedgerGeneratedIds generated_;
    ScopedBox<DynamicState> dynamic_;
    TraceChain fallback_;
    std::optional<ResourceBorrow<const EntityDescriptor>> descriptor_;
    typename Updates::iterator update_;
    Phase phase_ = Phase::Start;
    std::size_t index_ = 0;
    AsyncOperation<Result<std::optional<CompactRow>, Error>>* read_ = nullptr;
    AsyncOperation<Result<std::uint64_t, Error>>* mutation_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::execute_ledger_plan_internal(const EntityDataService<E>& service,
                                                           const EntityRuntimeState& root,
                                                           const LedgerObjectLocations& locations)
    -> AsyncOperation<Result<LedgerExecutionOutcome, DataServiceError<typename E::Error>>>& {
    using R = Result<LedgerExecutionOutcome, DataServiceError<typename E::Error>>;
    return make_operation<R, LedgerExecutionOperation<E>>(
        service.memory_resource(), service, root, locations);
}
}
