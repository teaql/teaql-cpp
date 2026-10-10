#pragma once
#include <teaql/runtime/detail/ledger_plan.hpp>
#include <teaql/runtime/detail/entity_graph_execution.hpp>
#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <bit>
#include <limits>

namespace teaql::detail {
template <class E>
class LedgerExecutionOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<LedgerGeneratedIds, Error>;
    using Signature = std::pair<String, String>;
    using Updates = std::pmr::map<Signature, Vector<EntityKey>>;
    enum class Phase { Start, Classify, Prepare, Delete, Insert, Update, Done };

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
            auto change_set = root_.current_change_set();
            for (const auto& [key, record] : change_set.changes()) {
                if (deleted_.contains(key)) {
                    continue;
                }
                EntityValues checked(std::allocator_arg, Allocator(r), record);
                checked.fields.try_emplace(String("id", r), key.id);
                changes_.emplace(clone_in(r, key), std::move(checked));
                keys_.emplace_back(key);
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
                    if (graph_unassigned(&id)) {
                        auto generated = context.next_id(entity);
                        if (!generated) {
                            return fail(std::move(generated).error());
                        }
                        id = Value(r, generated.value());
                        generated_.insert_or_assign(clone_in(r, key), Value(r, id));
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
                    command.batch_old_values.emplace_back(std::nullopt);
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
        phase_ = Phase::Done;
        return R::ok(r, std::move(generated_));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        read_ = nullptr;
        mutation_ = nullptr;
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
    }

private:
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
    -> AsyncOperation<Result<LedgerGeneratedIds, DataServiceError<typename E::Error>>>& {
    using R = Result<LedgerGeneratedIds, DataServiceError<typename E::Error>>;
    return make_operation<R, LedgerExecutionOperation<E>>(
        service.memory_resource(), service, root, locations);
}
}
