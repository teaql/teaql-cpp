#pragma once
#include <teaql/memory/arena.hpp>
#include <teaql/memory/strings.hpp>
#include <teaql/runtime/resources.hpp>
#include <teaql/runtime/registry.hpp>
#include <teaql/runtime/behavior.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/context_root.hpp>
#include <teaql/runtime/i18n.hpp>
#include <teaql/runtime/checker.hpp>
#include <teaql/runtime/fix_evidence.hpp>
#include <teaql/runtime/mutation_policy.hpp>
#include <teaql/runtime/business_clock.hpp>
#include <teaql/runtime/business_id.hpp>
#include <teaql/runtime/id.hpp>
#include <teaql/runtime/schema.hpp>
#include <teaql/runtime/locking.hpp>
#include <teaql/runtime/entity_reference.hpp>
#include <teaql/runtime/round_trip_reference_types.hpp>
#include <teaql/runtime/document_round_trip.hpp>
#include <teaql/runtime/logging.hpp>
#include <teaql/runtime/detail/context/identity.hpp>
#include <teaql/runtime/detail/json_hydration.hpp>
#include <teaql/data_service/dynamic_fields.hpp>
#include <exception>

namespace teaql {
class UserContext;
class ContextBinding;
class RuntimeModule;
class ContinuousPageCursorStore;
class IdSetStore;
class DataStore;
class DynamicFieldsProvider;
struct DynamicFieldBatch;
class EntityRuntimeState;
template <class E>
class EntityDataService;
template <class E>
class TransactionScope;
class InMemoryEntityGraphDecoderRegistry;
class LogFormatterFactory;
class LogManager;
class RuntimeTelemetry;
struct RuntimeOperation;
class FailOpenRuntimeTelemetryScope;
class RawAuditEventSink;
class SafeAuditEventSink;
struct RawAuditEvent;
struct MutationGovernanceSnapshot;
struct GraphNode;

namespace detail {
class GovernanceActivation;
class AuditActivation;
struct PendingAuditBatch;
struct ContextAuditAccess;
struct ContextEntityAccess;
struct ContextPaginationAccess;
class IdSetBuildRegistry;
struct GraphFixState;
class GraphFixActivation;
struct ContextFixAccess;
}

// Reject common accidental off-arena containers (std::string/vector).
// Foreign RAII wrappers use the explicitly named make_resource() entry point.
template <class T>
concept ArenaObject = std::uses_allocator_v<T, std::pmr::polymorphic_allocator<T>> ||
                      std::is_trivially_destructible_v<T>;

// Application-owned global lifetime; never a hidden singleton. Outlives every UserContext.
class GlobalContext final {
public:
    explicit GlobalContext(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource(),
                           std::size_t initial_block_size = 4096,
                           ContextLimits limits = {})
        : limits_(limits), runtime_identity_(detail::issue_runtime_identity()), upstream_(upstream),
          user_upstream_(upstream, limits.max_live_user_bytes),
          memory_(initial_block_size, upstream, nullptr, *this, limits.global) {
        if (limits.max_user_contexts > context_identity_limit) {
            throw std::invalid_argument("Context identity limit does not fit reference encoding");
        }
        memory_.set_close_guard(this, [](void* owner) noexcept {
            if (static_cast<GlobalContext*>(owner)->live_contexts_ != 0) {
                std::terminate();
            }
        });
    }

    GlobalContext(const GlobalContext&) = delete;
    GlobalContext& operator=(const GlobalContext&) = delete;
    GlobalContext(GlobalContext&&) = delete;
    GlobalContext& operator=(GlobalContext&&) = delete;

    ~GlobalContext() {
        // Detect lifetime inversion before freeing global metadata still borrowed by a context.
        if (live_contexts_ != 0) {
            std::terminate();
        }
        memory_.close();
    }

    template <ArenaObject T, class... Args>
    T& make(Args&&... args) {
        return memory_.make<T>(std::forward<Args>(args)...);
    }

    // Lifecycle-owned external resources; internal allocations remain the wrapper's responsibility.
    template <class T, class... Args>
    T& make_resource(Args&&... args) {
        return memory_.make_resource<T>(std::forward<Args>(args)...);
    }

    // Bootstrap references are explicitly rebound through an application UserContext.
    template <ArenaObject T, class... Args>
    Ref<T> make_ref(Args&&... args) {
        auto value = memory_.make_identified<T>(std::forward<Args>(args)...);
        return Ref<T>(detail::reference_id(runtime_identity_, 0, true, value.slot));
    }

    template <class T, class... Args>
    Ref<T> make_resource_ref(Args&&... args) {
        auto value = memory_.make_identified<T>(std::forward<Args>(args)...);
        return Ref<T>(detail::reference_id(runtime_identity_, 0, true, value.slot));
    }

    std::unique_ptr<UserContext> make_user_context(std::size_t initial_block_size = 4096);
    std::unique_ptr<UserContext> make_user_context(MemoryLimits limits,
                                                   std::size_t initial_block_size = 4096);

    std::uint32_t issued_contexts() const noexcept {
        return issued_contexts_;
    }

    const ContextLimits& limits() const noexcept {
        return limits_;
    }

    std::size_t live_user_bytes() const noexcept {
        return user_upstream_.used();
    }

    std::size_t peak_user_bytes() const noexcept {
        return user_upstream_.peak();
    }

    std::pmr::memory_resource* memory_resource() noexcept {
        return &memory_;
    }

    AllocationStats allocation_stats() const noexcept {
        return memory_.stats();
    }

    LogFormatterFactory& log_formatter_factory();
    LogManager& log_manager();

private:
    friend class UserContext;
    friend class detail::SchemaOperation;
    friend class detail::GovernanceActivation;
    friend class detail::AuditActivation;
    detail::PendingAuditBatch* active_audit_batch_ = nullptr;
    friend class detail::GraphFixActivation;
    detail::GraphFixState* active_graph_fix_ = nullptr;
    friend struct detail::ContextPaginationAccess;
    friend struct detail::ContextLockAccess;
    detail::LocalLocks* local_locks_ = nullptr;
    // Borrowed only while an owner-thread operation is being polled/cancelled.
    const MutationGovernanceSnapshot* active_mutation_governance_ = nullptr;
    friend bool detail::is_generated_schema_bootstrap(const UserContext&);
    bool generated_schema_bootstrap_ = false;
    ContextLimits limits_;
    std::uint32_t runtime_identity_;
    std::pmr::memory_resource* upstream_;
    detail::ByteBudgetResource user_upstream_;
    detail::ContextArena memory_;
    detail::ContextDirectory* directory_ = nullptr;
    std::uint32_t issued_contexts_ = 0;
    StringStorage* strings_ = nullptr;
    std::size_t live_contexts_ = 0;
    LogFormatterFactory* log_formatter_factory_ = nullptr;
    LogManager* log_manager_ = nullptr;
    AtomicCounterIdGenerator* local_id_generator_ = nullptr;
    detail::IdSetBuildRegistry* id_set_build_registry_ = nullptr;

    AtomicCounterIdGenerator& local_id_generator() {
        if (!local_id_generator_) {
            local_id_generator_ = &make<AtomicCounterIdGenerator>();
        }
        return *local_id_generator_;
    }

    std::pair<std::uint32_t, detail::ContextState&> register_context() {
        memory_.check_thread();
        if (memory_.closed() || memory_.closing()) {
            throw std::logic_error("GlobalContext is closing");
        }
        if (!directory_) {
            directory_ = &memory_.make_payload<detail::ContextDirectory>(limits_.max_user_contexts);
        }
        auto result = directory_->create();
        issued_contexts_ = result.first;
        return result;
    }

    Memory user_upstream() {
        memory_.check_thread();
        if (memory_.closed() || memory_.closing()) {
            throw std::logic_error("GlobalContext is closing");
        }
        return &user_upstream_;
    }

    StringStorage& strings() {
        if (!strings_) {
            strings_ = &memory_.make_payload<StringStorage>(memory_.limits().strings);
        }
        return *strings_;
    }
};

// Thread-confined owner; registered native work is quiesced before memory teardown.
class UserContext final {
public:
    explicit UserContext(GlobalContext& global, std::size_t initial_block_size = 4096)
        : UserContext(global, global.limits_.user, initial_block_size) {}

    UserContext(GlobalContext& global, MemoryLimits limits, std::size_t initial_block_size = 4096)
        : global_(global),
          memory_(initial_block_size, global.user_upstream(), &global.memory_, global, limits) {
        auto registration = global_.register_context();
        identity_ = registration.first;
        state_ = &registration.second;
        memory_.bind_state(*state_);
        ++global_.live_contexts_;
        try {
            user_identifier_ =
                &memory_.make_payload<String>(detail::default_user_identifier(&memory_));
            trace_id_ = &memory_.make_payload<String>(detail::default_trace_id(&memory_));
        } catch (...) {
            memory_.close();
            --global_.live_contexts_;
            throw;
        }
        state_->phase = detail::ContextState::Phase::Open;
        state_->published = true;
    }

    UserContext(const UserContext&) = delete;
    UserContext& operator=(const UserContext&) = delete;
    UserContext(UserContext&&) = delete;
    UserContext& operator=(UserContext&&) = delete;

    ~UserContext() {
        close();
        --global_.live_contexts_;
    }

    template <ArenaObject T, class... Args>
    T& make(Args&&... args) const {
        return const_cast<detail::ContextArena&>(memory_).make<T>(std::forward<Args>(args)...);
    }

    template <class Factory>
        requires std::constructible_from<Factory, UserContext&>
    Factory queries() {
        check_public();
        return Factory(*this);
    }

    template <class T, class... Args>
    T& make_resource(Args&&... args) const {
        return const_cast<detail::ContextArena&>(memory_).make_resource<T>(
            std::forward<Args>(args)...);
    }

    // Checked application factories. The raw make() methods above remain kernel borrows.
    template <ArenaObject T, class... Args>
    Ref<T> make_ref(Args&&... args) const {
        check_public();
        auto value = const_cast<detail::ContextArena&>(memory_).make_identified<T>(
            std::forward<Args>(args)...);
        return Ref<T>(scope_id(false, value.slot));
    }

    template <class T, class... Args>
    Ref<T> make_resource_ref(Args&&... args) const {
        check_public();
        auto value = const_cast<detail::ContextArena&>(memory_).make_identified<T>(
            std::forward<Args>(args)...);
        return Ref<T>(scope_id(false, value.slot));
    }

    template <ArenaObject T, class... Args>
    Ref<T> make_global(Args&&... args) const {
        return make_global_resource<T>(std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    Ref<T> make_global_resource(Args&&... args) const {
        check_public();
        auto value = global_.memory_.make_identified<T>(std::forward<Args>(args)...);
        return Ref<T>(scope_id(true, value.slot));
    }

    template <class T>
    T& get(Ref<T> reference, std::source_location access = std::source_location::current()) const {
        auto& arena = reference_arena(reference.id(), access);
        return *static_cast<T*>(arena.resolve(
            detail::reference_slot(reference.id()), typeid(T), reference.id(), access));
    }

    template <class T>
    void destroy(Ref<T> reference,
                 std::source_location access = std::source_location::current()) const {
        auto& arena = reference_arena(reference.id(), access);
        arena.destroy(detail::reference_slot(reference.id()), typeid(T), reference.id(), access);
    }

    template <class T>
    Ref<T> rebind_global(Ref<T> reference,
                         std::source_location access = std::source_location::current()) const {
        check_global_rebind(reference.id(), access);
        auto slot = detail::reference_slot(reference.id());
        (void)global_.memory_.resolve(slot, typeid(T), reference.id(), access);
        return Ref<T>(scope_id(true, slot));
    }

    StringRef save_string(std::string_view text) const {
        check_public();
        return StringRef(scope_id(false), strings().save(text));
    }

    StringRef intern_string(std::string_view text) const {
        check_public();
        return StringRef(scope_id(false), strings().intern(text));
    }

    StringRef save_global_string(std::string_view text) const {
        check_public();
        return StringRef(scope_id(true), global_.strings().save(text));
    }

    StringRef intern_global_string(std::string_view text) const {
        check_public();
        return StringRef(scope_id(true), global_.strings().intern(text));
    }

    StringRef literal_string(StaticString text) const {
        check_public();
        return StringRef(scope_id(true), text.view());
    }

    std::string_view get(StringRef reference,
                         std::source_location access = std::source_location::current()) const {
        (void)reference_arena(reference.scope_, access);
        return {reference.data_, reference.size_};
    }

    StringRef rebind_global(StringRef reference,
                            std::source_location access = std::source_location::current()) const {
        check_global_rebind(reference.scope_, access);
        return StringRef(scope_id(true), {reference.data_, reference.size_});
    }

    BytesRef save_bytes(std::span<const std::byte> bytes) const {
        return save_bytes_in(bytes, false);
    }

    BytesRef save_global_bytes(std::span<const std::byte> bytes) const {
        return save_bytes_in(bytes, true);
    }

    std::span<const std::byte>
    get(BytesRef reference, std::source_location access = std::source_location::current()) const {
        (void)reference_arena(reference.scope_, access);
        return {reference.data_, reference.size_};
    }

    BytesRef rebind_global(BytesRef reference,
                           std::source_location access = std::source_location::current()) const {
        check_global_rebind(reference.scope_, access);
        return BytesRef(scope_id(true), {reference.data_, reference.size_});
    }

    StringStorageStats string_storage_stats(bool global = false) const {
        check_public();
        auto* storage = global ? global_.strings_ : strings_;
        return storage ? storage->stats() : StringStorageStats{};
    }

    const MemoryLimits& memory_limits() const noexcept {
        return memory_.limits();
    }

    std::uint32_t identity() const noexcept {
        return identity_;
    }

    std::pmr::memory_resource* memory_resource() const noexcept {
        return const_cast<detail::ContextArena*>(&memory_);
    }

    AllocationStats allocation_stats() const noexcept {
        return memory_.stats();
    }

    bool closed() const noexcept {
        return memory_.closed();
    }

    void close(std::source_location where = std::source_location::current()) noexcept {
        memory_.close(where);
    }

    Language language() const {
        check_public();
        return language_;
    }

    void set_language(Language language) {
        check_public();
        ensure_mutable();
        language_ = language;
    }

    UserContext& with_language(Language language) & {
        set_language(language);
        return *this;
    }

    RuntimeHookResult set_language_code(std::string_view);

    RuntimeHookResult set_locale_code(std::string_view code) {
        return set_language_code(code);
    }

    template <class Owner>
    void set_i18n_catalog(Owner& origin, I18nCatalog& catalog) {
        check_public();
        ensure_mutable();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), catalog);
        if (!registered.slot) {
            throw std::invalid_argument("I18n catalog must be a managed Context object");
        }
        i18n_catalog_.emplace(std::allocator_arg,
                              Allocator(memory_resource()),
                              catalog,
                              *registered.owner,
                              registered.slot);
    }

    void set_i18n_catalog(Ref<I18nCatalog> catalog) {
        auto& origin = reference_arena(catalog.id(), std::source_location::current());
        ensure_mutable();
        i18n_catalog_.emplace(std::allocator_arg,
                              Allocator(memory_resource()),
                              get(catalog),
                              origin,
                              detail::reference_slot(catalog.id()));
    }

    template <class Owner>
    UserContext& with_i18n_catalog(Owner& origin, I18nCatalog& catalog) & {
        set_i18n_catalog(origin, catalog);
        return *this;
    }

    UserContext& with_i18n_catalog(Ref<I18nCatalog> catalog) & {
        set_i18n_catalog(catalog);
        return *this;
    }

    void translate_check_results(CheckResults&) const;

    template <class Owner, class R>
        requires std::derived_from<R, CheckerRegistry>
    void set_checker_registry(Owner& origin, R& registry) {
        check_public();
        ensure_mutable();
        auto registered = detail::checker_resource(memory_, origin, registry);
        checker_registry_.emplace(std::allocator_arg,
                                  Allocator(memory_resource()),
                                  registry,
                                  *registered.owner,
                                  registered.slot);
    }

    template <class R>
        requires std::derived_from<R, CheckerRegistry>
    void set_checker_registry(Ref<R> registry) {
        auto& origin = reference_arena(registry.id(), std::source_location::current());
        ensure_mutable();
        (void)detail::ContextResources::entry(
            memory_, &origin, get(registry), detail::reference_slot(registry.id()));
        checker_registry_.emplace(std::allocator_arg,
                                  Allocator(memory_resource()),
                                  get(registry),
                                  origin,
                                  detail::reference_slot(registry.id()));
    }

    template <class Owner, class R>
        requires std::derived_from<R, CheckerRegistry>
    UserContext& with_checker_registry(Owner& origin, R& registry) & {
        set_checker_registry(origin, registry);
        return *this;
    }

    template <class R>
        requires std::derived_from<R, CheckerRegistry>
    UserContext& with_checker_registry(Ref<R> registry) & {
        set_checker_registry(registry);
        return *this;
    }

    UserContext& with_module(const RuntimeModule&) &;

    bool has_checker(std::string_view entity) const;
    Timestamp fix_time() const;
    void record_fix_evidence(const FixEvidence&) const;
    void record_fix_evidence(FixEvidence&&) const;
    Vector<FixEvidence> last_fix_evidence() const;
    std::string_view trace_id() const;
    void set_trace_id(std::string_view);

    UserContext& with_trace_id(std::string_view value) & {
        set_trace_id(value);
        return *this;
    }

    template <class Owner, class T>
        requires std::derived_from<T, MutationPolicyRegistry>
    void set_mutation_policy_registry(Owner& origin, T& value) {
        set_checked_resource(mutation_policy_registry_, origin, value);
    }

    template <class T>
        requires std::derived_from<T, MutationPolicyRegistry>
    void set_mutation_policy_registry(Ref<T> value) {
        set_checked_resource(mutation_policy_registry_, value);
    }

    template <class Owner, class T>
        requires std::derived_from<T, MutationPolicyRegistry>
    UserContext& with_mutation_policy_registry(Owner& origin, T& value) & {
        set_mutation_policy_registry(origin, value);
        return *this;
    }

    template <class T>
        requires std::derived_from<T, MutationPolicyRegistry>
    UserContext& with_mutation_policy_registry(Ref<T> value) & {
        set_mutation_policy_registry(value);
        return *this;
    }

    template <class Owner, class T>
        requires std::derived_from<T, MutationPolicyApprovalProvider>
    void set_mutation_policy_approval_provider(Owner& origin, T& value) {
        set_checked_resource(mutation_policy_approval_provider_, origin, value);
    }

    template <class T>
        requires std::derived_from<T, MutationPolicyApprovalProvider>
    void set_mutation_policy_approval_provider(Ref<T> value) {
        set_checked_resource(mutation_policy_approval_provider_, value);
    }

    template <class Owner, class T>
        requires std::derived_from<T, MutationPolicyApprovalProvider>
    UserContext& with_mutation_policy_approval_provider(Owner& origin, T& value) & {
        set_mutation_policy_approval_provider(origin, value);
        return *this;
    }

    template <class T>
        requires std::derived_from<T, MutationPolicyApprovalProvider>
    UserContext& with_mutation_policy_approval_provider(Ref<T> value) & {
        set_mutation_policy_approval_provider(value);
        return *this;
    }

    template <class Owner, class T>
        requires std::derived_from<T, MutationGovernanceSink>
    void set_mutation_governance_sink(Owner& origin, T& value) {
        set_checked_resource(mutation_governance_sink_, origin, value);
    }

    template <class T>
        requires std::derived_from<T, MutationGovernanceSink>
    void set_mutation_governance_sink(Ref<T> value) {
        set_checked_resource(mutation_governance_sink_, value);
    }

    template <class Owner, class T>
        requires std::derived_from<T, MutationGovernanceSink>
    UserContext& with_mutation_governance_sink(Owner& origin, T& value) & {
        set_mutation_governance_sink(origin, value);
        return *this;
    }

    template <class T>
        requires std::derived_from<T, MutationGovernanceSink>
    UserContext& with_mutation_governance_sink(Ref<T> value) & {
        set_mutation_governance_sink(value);
        return *this;
    }

    Result<MutationGovernanceSnapshot, RuntimeError>
    review_mutation_plan(const MutationPlan&) const;
    RuntimeHookResult check_and_fix_values(std::string_view entity, EntityValues&) const;
    RuntimeHookResult
    check_and_fix_values_at(std::string_view entity, EntityValues&, const ObjectLocation&) const;

    template <class Owner, class T>
        requires std::derived_from<T, ContinuousPageCursorStore>
    void set_continuous_page_cursor_store(Owner& origin, T& store) {
        ensure_mutable();
        detail::ContextResources::entry(memory_, origin.memory_resource(), store);
        continuous_page_cursor_borrow_.emplace(std::allocator_arg,
                                               Allocator(memory_resource()),
                                               static_cast<ContinuousPageCursorStore&>(store),
                                               detail::resource_arena(origin.memory_resource()));
        continuous_page_cursor_store_ = &store;
    }

    template <class Owner, class T>
        requires std::derived_from<T, IdSetStore>
    void set_id_set_store(Owner& origin, T& store) {
        ensure_mutable();
        detail::ContextResources::entry(memory_, origin.memory_resource(), store);
        id_set_store_borrow_.emplace(std::allocator_arg,
                                     Allocator(memory_resource()),
                                     static_cast<IdSetStore&>(store),
                                     detail::resource_arena(origin.memory_resource()));
        id_set_store_ = &store;
    }

    std::optional<String> continuous_page_plan() const;
    std::optional<String> continuous_page_cursor_id() const;
    std::optional<String> id_set_plan() const;
    std::optional<std::uint64_t> id_set_count() const;
    UserContext& with_active_root(std::string_view entity_type, std::uint64_t id);
    RuntimeHookResult set_generated_bootstrap_active_root(std::string_view entity_type,
                                                          std::uint64_t id) const;
    bool try_local_lock(std::string_view key,
                        std::uint64_t timeout_millis,
                        std::uint64_t expire_millis) const;
    void unlock_local(std::string_view key) const;
    AsyncOperation<bool>& try_remote_lock(std::string_view key,
                                          std::uint64_t timeout_millis,
                                          std::uint64_t expire_millis) const;
    AsyncOperation<bool>& unlock_remote(std::string_view key) const;

    template <class Owner, class T>
        requires std::derived_from<T, RemoteLockProvider>
    void set_remote_lock_provider(Owner& origin, T& provider) {
        insert_resource(origin, static_cast<RemoteLockProvider&>(provider));
    }

    template <class T>
        requires std::derived_from<T, RemoteLockProvider>
    void set_remote_lock_provider(Ref<T> reference) {
        auto& origin = reference_arena(reference.id(), std::source_location::current());
        resources().insert(memory_, &origin, static_cast<RemoteLockProvider&>(get(reference)));
    }

    Result<ResourceBorrow<const ContextEntityRef>, ContextRootError>
    require_active_root(std::string_view expected_entity_type) const;

    // Each operation gets an independent ledger and shares only this User's graph anchor.
    EntityRuntimeState entity_runtime_state() const;

    template <class Owner, class T>
        requires std::derived_from<T, EntityReferenceCodec>
    void set_entity_reference_codec(Owner& owner, T& codec) {
        check_public();
        insert_resource(owner, static_cast<EntityReferenceCodec&>(codec));
    }

    template <class T>
        requires std::derived_from<T, EntityReferenceCodec>
    void set_entity_reference_codec(Ref<T> reference) {
        auto& origin = reference_arena(reference.id(), std::source_location::current());
        resources().insert(memory_, &origin, static_cast<EntityReferenceCodec&>(get(reference)));
    }

    template <class... Args>
    UserContext& with_entity_reference_codec(Args&&... args) {
        set_entity_reference_codec(std::forward<Args>(args)...);
        return *this;
    }

    EntityReferenceEncodeResult encode_entity_reference(std::string_view entity_type,
                                                        std::uint64_t id,
                                                        std::int64_t version,
                                                        std::string_view purpose,
                                                        ReferenceLifetime) const;
    EntityReferenceDecodeResult decode_entity_reference(std::string_view token,
                                                        std::string_view expected_entity_type,
                                                        std::string_view purpose) const;

    template <class Owner>
    void set_round_trip_reference_runtime(Owner& owner, ContextBoundReferenceRuntime& runtime) {
        check_public();
        insert_resource(owner, runtime);
    }

    template <class T>
        requires std::same_as<T, ContextBoundReferenceRuntime>
    void set_round_trip_reference_runtime(Ref<T> reference) {
        insert_resource(reference);
    }

    template <class... Args>
    UserContext& with_round_trip_reference_runtime(Args&&... args) {
        set_round_trip_reference_runtime(std::forward<Args>(args)...);
        return *this;
    }

    void set_trusted_reference_principal(const TrustedReferencePrincipal&);

    template <class Owner, class T>
        requires std::derived_from<T, ContextBoundDocumentService>
    void set_context_bound_document_service(Owner& owner, T& service) {
        check_public();
        insert_resource(owner, static_cast<ContextBoundDocumentService&>(service));
    }

    template <class T>
        requires std::derived_from<T, ContextBoundDocumentService>
    void set_context_bound_document_service(Ref<T> reference) {
        auto& origin = reference_arena(reference.id(), std::source_location::current());
        resources().insert(
            memory_, &origin, static_cast<ContextBoundDocumentService&>(get(reference)));
    }

    template <class... Args>
    UserContext& with_context_bound_document_service(Args&&... args) {
        set_context_bound_document_service(std::forward<Args>(args)...);
        return *this;
    }

    AsyncOperation<DocumentResult<ContextualDocument>>& open_document(DocumentOpenRequest) const;
    AsyncOperation<DocumentResult<AcceptedContextualDocument>>&
        accept_document(DocumentAcceptRequest) const;
    DocumentResult<String> issue_document_snapshot(const DocumentSnapshot&,
                                                   ReferenceLifetime) const;
    DocumentResult<VerifiedDocumentSnapshot>
    consume_document_snapshot(std::string_view token,
                              std::string_view expected_aggregate_type,
                              std::string_view expected_purpose) const;

    UserContext& with_trusted_reference_principal(const TrustedReferencePrincipal& principal) {
        set_trusted_reference_principal(principal);
        return *this;
    }

    RoundTripResult<ReferenceMode> reference_mode() const;
    RoundTripResult<std::optional<ReferenceStartupNotice>> reference_startup_notice() const;
    RoundTripResult<ExternalEntityReference>
    reference_for(const ReferenceIdentity&, const ReferenceDocumentScope&, ReferenceLifetime) const;
    RoundTripResult<ResolvedEntityReference>
    resolve_reference(const ExternalEntityReference&,
                      std::string_view expected_entity_type,
                      const ReferenceDocumentScope&) const;
    RoundTripResult<Json> serialize_reference(const ExternalEntityReference&) const;
    RoundTripResult<ExternalEntityReference> deserialize_reference(const Json&) const;

    AsyncOperation<std::optional<Value>>& get_in_store(std::string_view key) const;
    AsyncOperation<std::monostate>& put_in_store(
        std::string_view key, Value value, std::optional<std::uint64_t> timeout_seconds) const;
    AsyncOperation<std::monostate>& clear_in_store(std::string_view key) const;

    template <class T>
        requires(!std::same_as<std::remove_cvref_t<T>, Value> &&
                 std::constructible_from<Value, Memory, T>)
    AsyncOperation<std::monostate>& put_in_store(
        std::string_view key, T&& value, std::optional<std::uint64_t> timeout_seconds) const {
        return put_in_store(key, Value(memory_resource(), std::forward<T>(value)), timeout_seconds);
    }

    template <class Owner, class T>
        requires std::derived_from<T, DataStore>
    void set_data_store(Owner& origin, T& store) {
        check_public();
        insert_resource(origin, static_cast<DataStore&>(store));
    }

    template <class T>
        requires std::derived_from<T, DataStore>
    void set_data_store(Ref<T> reference) {
        auto& origin = reference_arena(reference.id(), std::source_location::current());
        resources().insert(memory_, &origin, static_cast<DataStore&>(get(reference)));
    }

    // Explicit origin makes registration a checked Context borrow, never a
    // second owner. Allocate the object with origin.make/make_resource first.
    template <class Owner, class P>
    void set_dynamic_fields_provider(Owner& origin, P& provider) {
        insert_resource(origin, static_cast<DynamicFieldsProvider&>(provider));
    }

    Result<DynamicFieldStreamPlan, DynamicFieldError>
    prepare_dynamic_field_stream(const DynamicFieldStore&,
                                 std::string_view owner_type,
                                 const DynamicFieldSelection&,
                                 const QueryIntent&) const;
    Result<const DynamicFieldsProvider*, RuntimeError> dynamic_graph_provider() const;
    AsyncOperation<Result<DynamicFieldBatch, DynamicFieldError>>&
    load_dynamic_fields(std::string_view owner_type,
                        std::span<const std::uint64_t>,
                        const DynamicFieldSelection&,
                        const QueryIntent&) const;
    DynamicStorageFuture<std::monostate>& ensure_dynamic_fields_schema() const;

    template <class Owner, class E>
    void register_executor(Owner&, E&);

    template <class E>
    void register_executor(Ref<E>);

    template <class Owner, class T>
    void insert_resource(Owner& origin, T& resource) {
        resources().insert(memory_, origin.memory_resource(), resource);
    }

    template <class T>
    void insert_resource(Ref<T> reference) {
        auto& origin = reference_arena(reference.id(), std::source_location::current());
        resources().insert(memory_, &origin, get(reference));
    }

    template <class Owner, class T>
    void insert_named_resource(std::string_view name, Owner& origin, T& resource) {
        resources().insert_named(memory_, origin.memory_resource(), name, resource);
    }

    template <class T>
    void insert_named_resource(std::string_view name, Ref<T> reference) {
        auto& origin = reference_arena(reference.id(), std::source_location::current());
        resources().insert_named(memory_, &origin, name, get(reference));
    }

    template <class T>
    T* get_resource() {
        ensure_open();
        auto* entry = resources_ ? resources_->find<T>() : nullptr;
        return entry ? static_cast<T*>(entry->object) : nullptr;
    }

    template <class T>
    const T* get_resource() const {
        ensure_open();
        auto* entry = resources_ ? resources_->find<T>() : nullptr;
        return entry ? static_cast<const T*>(entry->object) : nullptr;
    }

    template <class T>
    T* get_named_resource(std::string_view name) {
        ensure_open();
        auto* entry = resources_ ? resources_->find_named<T>(name) : nullptr;
        return entry ? static_cast<T*>(entry->object) : nullptr;
    }

    template <class T>
    const T* get_named_resource(std::string_view name) const {
        ensure_open();
        auto* entry = resources_ ? resources_->find_named<T>(name) : nullptr;
        return entry ? static_cast<const T*>(entry->object) : nullptr;
    }

    template <class T>
    Result<ResourceBorrow<T>, ContextError> require_resource() {
        ensure_open();
        return required<T>(resources_ ? resources_->find<T>() : nullptr,
                           ContextError::Kind::MissingTypedResource,
                           detail::resource_type_name<T>());
    }

    template <class T>
    Result<ResourceBorrow<const T>, ContextError> require_resource() const {
        ensure_open();
        return required<const T>(resources_ ? resources_->find<T>() : nullptr,
                                 ContextError::Kind::MissingTypedResource,
                                 detail::resource_type_name<T>());
    }

    template <class T>
    Result<ResourceBorrow<T>, ContextError> require_named_resource(std::string_view name) {
        ensure_open();
        return required<T>(resources_ ? resources_->find_named<T>(name) : nullptr,
                           ContextError::Kind::MissingResource,
                           name);
    }

    template <class T>
    Result<ResourceBorrow<const T>, ContextError>
    require_named_resource(std::string_view name) const {
        ensure_open();
        return required<const T>(resources_ ? resources_->find_named<T>(name) : nullptr,
                                 ContextError::Kind::MissingResource,
                                 name);
    }

    template <class T>
    void put_local(std::string_view key, T&& value) {
        resources().put_local(key, Value(&memory_, std::forward<T>(value)));
    }

    const Value* local(std::string_view key) const {
        ensure_open();
        return resources_ ? resources_->local(key) : nullptr;
    }

    std::optional<Value> remove_local(std::string_view key) {
        ensure_mutable();
        return resources_ ? resources_->remove_local(key) : std::optional<Value>{};
    }

    template <class Owner, class T>
        requires std::derived_from<T, RuntimeTelemetry>
    void set_runtime_telemetry(Owner& origin, T& telemetry) {
        ensure_mutable();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), telemetry);
        if (!registered.slot) {
            throw std::invalid_argument("Context telemetry must be a managed Context object");
        }
        runtime_telemetry_.emplace(std::allocator_arg,
                                   Allocator(memory_resource()),
                                   static_cast<const RuntimeTelemetry&>(telemetry),
                                   *registered.owner,
                                   registered.slot);
    }

    template <class Owner, class T>
        requires std::derived_from<T, RuntimeTelemetry>
    UserContext& with_runtime_telemetry(Owner& origin, T& telemetry) {
        set_runtime_telemetry(origin, telemetry);
        return *this;
    }

    const RuntimeTelemetry& runtime_telemetry() const;
    bool runtime_telemetry_is_noop() const;
    FailOpenRuntimeTelemetryScope& start_runtime_operation(const RuntimeOperation&) const;

    std::optional<std::string_view> user_identifier() const {
        ensure_open();
        return user_identifier_ ? std::optional<std::string_view>(*user_identifier_) : std::nullopt;
    }

    void set_user_identifier(std::string_view value) {
        set_user_identifier_option(value);
    }

    void set_user_identifier_option(std::optional<std::string_view> value) {
        ensure_mutable();
        if (value) {
            user_identifier_ = &make<String>(*value);
        } else {
            user_identifier_ = nullptr;
        }
    }

    UserContext& with_user_identifier(std::string_view value) {
        set_user_identifier(value);
        return *this;
    }

    UserContext& with_user_identifier_option(std::optional<std::string_view> value) {
        set_user_identifier_option(value);
        return *this;
    }

    std::optional<std::string_view> timezone() const {
        ensure_open();
        return timezone_ ? std::string_view(*timezone_) : std::string_view("UTC");
    }

    void set_timezone(std::string_view value) {
        ensure_mutable();
        timezone_ = &make<String>(value);
    }

    UserContext& with_timezone(std::string_view value) & {
        set_timezone(value);
        return *this;
    }

    UserContext& with_sql_log_options(SqlLogOptions value) {
        set_sql_log_options(value);
        return *this;
    }

    void set_sql_log_options(SqlLogOptions value) {
        ensure_mutable();
        sql_log_options_ = value;
    }

    void enable_select_sql_log() {
        ensure_mutable();
        sql_log_options_.select = true;
    }

    void enable_mutation_sql_log() {
        ensure_mutable();
        sql_log_options_.mutation = true;
    }

    void disable_select_sql_log() {
        ensure_mutable();
        sql_log_options_.select = false;
    }

    void disable_mutation_sql_log() {
        ensure_mutable();
        sql_log_options_.mutation = false;
    }

    void enable_all_sql_log() {
        set_sql_log_options(SqlLogOptions::all());
    }

    void disable_sql_log() {
        set_sql_log_options(SqlLogOptions::disabled());
        clear_sql_logs();
    }

    SqlLogOptions sql_log_options() const {
        ensure_open();
        return sql_log_options_;
    }

    Vector<SqlLogEntry> sql_logs() const;
    void clear_sql_logs() const;
    // Internal runtime/provider diagnostic bridge, including failure metadata.
    void record_metadata_log(const ExecutionMetadata&) const;

    template <class Owner, class T>
        requires std::derived_from<T, RawAuditEventSink>
    void set_event_sink(Owner& origin, T& sink) {
        ensure_mutable();
        auto registered = detail::ContextResources::entry(memory_, origin.memory_resource(), sink);
        event_sink_borrow_.emplace(std::allocator_arg,
                                   Allocator(memory_resource()),
                                   sink,
                                   *registered.owner,
                                   registered.slot);
        event_sink_ = &sink;
    }

    template <class Owner, class T>
        requires std::derived_from<T, RawAuditEventSink>
    UserContext& with_event_sink(Owner& origin, T& sink) {
        set_event_sink(origin, sink);
        return *this;
    }

    template <class Owner, class T>
        requires std::derived_from<T, SafeAuditEventSink>
    void set_custom_event_sink(Owner& origin, T& sink) {
        ensure_mutable();
        auto registered = detail::ContextResources::entry(memory_, origin.memory_resource(), sink);
        custom_event_sink_borrow_.emplace(std::allocator_arg,
                                          Allocator(memory_resource()),
                                          sink,
                                          *registered.owner,
                                          registered.slot);
        custom_event_sink_ = &sink;
    }

    template <class Owner, class T>
        requires std::derived_from<T, SafeAuditEventSink>
    UserContext& with_custom_event_sink(Owner& origin, T& sink) {
        set_custom_event_sink(origin, sink);
        return *this;
    }

    Result<std::monostate, RuntimeError> send_event(const RawAuditEvent&) const;

    template <class E, class T, class F>
    AsyncOperation<Result<T, RuntimeError>>& execute_in_transaction(F&& operation) const;

    // Source Send entry; allocation and polling retain Context owner-thread rules.
    template <class E, class T, class F>
    AsyncOperation<Result<T, RuntimeError>>& execute_in_send_transaction(F&& operation) const;
    std::span<const GraphNode> initial_graphs() const;
    std::span<const GraphNode> root_graphs() const;
    void set_initial_graphs(std::span<const GraphNode>);
    void set_root_graphs(std::span<const GraphNode>);

    template <class E>
    Result<ResourceBorrow<EntityDataService<E>>, ContextError>
    entity_data_service(std::string_view entity) const;
    void set_entity_graph_decoder_registry(const InMemoryEntityGraphDecoderRegistry&);

    // Presentation hydration against installed metadata; decoded data grants no save authority.
    template <Entity T>
    Result<T, EntityError> decode_json_entity(const Json& value) const {
        check_public();
        auto r = memory_resource();
        detail::JsonReadSession session(*this);
        auto descriptor = session.descriptor(T::entity_descriptor(r));
        if (!descriptor) {
            return Result<T, EntityError>::err(r, std::move(descriptor).error());
        }
        auto layout = detail::require_json_layout<T>(r);
        if (!layout) {
            return Result<T, EntityError>::err(r, std::move(layout).error());
        }
        std::optional<EntityRuntimeState> state;
        auto row =
            session.decode(*descriptor.value(), value, T::supports_dynamic_property_load(), state);
        if (!row) {
            return Result<T, EntityError>::err(r, std::move(row).error());
        }
        if (state) {
            return T::from_compact_row_with_context(r, std::move(row).value(), AnyRef(*state));
        }
        return T::from_compact_row(r, std::move(row).value());
    }

    template <Entity T>
    Result<Vector<T>, EntityError> decode_json_entities(const Json& value) const {
        check_public();
        auto r = memory_resource();
        using Output = Result<Vector<T>, EntityError>;
        detail::JsonReadSession session(*this);
        auto descriptor = session.descriptor(T::entity_descriptor(r));
        if (!descriptor) {
            return Output::err(r, std::move(descriptor).error());
        }
        auto layout = detail::require_json_layout<T>(r);
        if (!layout) {
            return Output::err(r, std::move(layout).error());
        }
        const auto* array = std::get_if<Json::Array>(&value.data);
        if (!array) {
            return Output::err(
                r, detail::json_input_error(r, T::ENTITY_NAME, "expected entity array"));
        }
        Vector<T> output(r);
        output.reserve(array->size());
        for (const auto& input : *array) {
            std::optional<EntityRuntimeState> state;
            auto row = session.decode(
                *descriptor.value(), input, T::supports_dynamic_property_load(), state);
            if (!row) {
                return Output::err(r, std::move(row).error());
            }
            auto entity =
                state ? T::from_compact_row_with_context(r, std::move(row).value(), AnyRef(*state))
                      : T::from_compact_row(r, std::move(row).value());
            if (!entity) {
                return Output::err(r, std::move(entity).error());
            }
            output.emplace_back(std::move(entity).value());
        }
        return Output::ok(r, std::move(output));
    }

    template <class Owner, class T>
        requires std::derived_from<T, EntityDataServiceBehaviorRegistry>
    void set_entity_data_service_behavior_registry(Owner& origin, T& registry) {
        ensure_mutable();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), registry);
        entity_behaviors_borrow_.emplace(std::allocator_arg,
                                         Allocator(memory_resource()),
                                         registry,
                                         *registered.owner,
                                         registered.slot);
        entity_behaviors_ = &registry;
    }

    template <class Owner, class T>
        requires std::derived_from<T, EntityDataServiceBehaviorRegistry>
    UserContext& with_entity_data_service_behavior_registry(Owner& origin, T& registry) {
        set_entity_data_service_behavior_registry(origin, registry);
        return *this;
    }

    std::optional<EntityBehaviorBorrow> entity_data_service_behavior(std::string_view entity) const;

    template <class Owner, class T>
        requires std::derived_from<T, RequestPolicy>
    void set_request_policy(Owner& origin, T& policy) {
        ensure_mutable();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), policy);
        request_policy_borrow_.emplace(std::allocator_arg,
                                       Allocator(memory_resource()),
                                       policy,
                                       *registered.owner,
                                       registered.slot);
        request_policy_ = &policy;
    }

    template <class Owner, class T>
        requires std::derived_from<T, RequestPolicy>
    UserContext& with_request_policy(Owner& origin, T& policy) {
        set_request_policy(origin, policy);
        return *this;
    }

    void clear_request_policy() {
        ensure_mutable();
        request_policy_borrow_.reset();
        request_policy_ = nullptr;
    }

    template <class Owner, class T>
        requires std::derived_from<T, BusinessClock>
    void set_business_clock(Owner& origin, T& clock) {
        ensure_mutable();
        check_public();
        auto registered = detail::ContextResources::entry(memory_, origin.memory_resource(), clock);
        if (!registered.slot) {
            throw std::invalid_argument("Business clock must be a managed Context object");
        }
        business_clock_.emplace(std::allocator_arg,
                                Allocator(memory_resource()),
                                clock,
                                *registered.owner,
                                registered.slot);
    }

    template <class Owner, class T>
        requires std::derived_from<T, BusinessClock>
    UserContext& with_business_clock(Owner& origin, T& clock) {
        set_business_clock(origin, clock);
        return *this;
    }

    UtcDateTime business_time() const {
        ensure_open();
        if (!business_clock_) {
            return SystemBusinessClock{}.business_time();
        }
        const auto& clock = business_clock_->get();
        Arena::Quiescence active;
        if (!memory_.closing()) {
            active.attach(const_cast<detail::ContextArena&>(memory_), nullptr, [](void*) noexcept {
                std::terminate();
            });
        }
        return clock.business_time();
    }

    Date business_date() const {
        if (auto* date = get_resource<BusinessDate>()) {
            return date->value;
        }
        if (!business_clock_) {
            return SystemBusinessClock{}.business_date();
        }
        const auto& clock = business_clock_->get();
        Arena::Quiescence active;
        if (!memory_.closing()) {
            active.attach(const_cast<detail::ContextArena&>(memory_), nullptr, [](void*) noexcept {
                std::terminate();
            });
        }
        return clock.business_date();
    }

    template <class Owner>
    void set_business_id_service(Owner& origin, BusinessIdService& service) {
        insert_resource(origin, service);
    }

    template <class Owner>
    UserContext& with_business_id_service(Owner& origin, BusinessIdService& service) {
        set_business_id_service(origin, service);
        return *this;
    }

    Result<ResourceBorrow<const BusinessIdService>, ContextError> business_ids() const {
        return require_resource<BusinessIdService>();
    }

    template <class Owner, class T>
        requires std::derived_from<T, BusinessIdKeyProvider>
    Result<std::monostate, ContextError> set_business_id_key_provider(Owner& origin, T& provider) {
        ensure_mutable();
        auto service = business_ids();
        using R = Result<std::monostate, ContextError>;
        if (!service) {
            return R::err(&memory_, detail::clone_in(&memory_, service.error()));
        }
        auto& replacement = make<BusinessIdService>(service.value().get());
        replacement.with_key_provider(origin, provider);
        insert_resource(*this, replacement);
        return R::ok(&memory_, {});
    }

    template <class Owner, class T>
        requires std::derived_from<T, BusinessIdAllocator> &&
                 std::derived_from<T, BusinessIdSchemaContributor>
    void set_business_id_infrastructure(Owner& origin, T& allocator) {
        ensure_mutable();
        auto& service = make<BusinessIdService>(origin, allocator);
        auto& schema = make<detail::BusinessIdSchemaService>(origin, allocator);
        insert_resource(*this, service);
        insert_resource(*this, schema);
    }

    template <class Owner, class T>
        requires std::derived_from<T, MetadataStore>
    void set_metadata(Owner& origin, T& metadata) {
        ensure_mutable();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), metadata);
        metadata_borrow_.emplace(std::allocator_arg,
                                 Allocator(memory_resource()),
                                 metadata,
                                 *registered.owner,
                                 registered.slot);
        metadata_ = &metadata;
    }

    template <class Owner, class T>
        requires std::derived_from<T, MetadataStore>
    UserContext& with_metadata(Owner& origin, T& metadata) {
        set_metadata(origin, metadata);
        return *this;
    }

    template <class Owner, class T>
        requires std::derived_from<T, EntityRegistry>
    void set_entity_registry(Owner& origin, T& registry) {
        ensure_mutable();
        check_public();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), registry);
        if (!registered.slot) {
            throw std::invalid_argument("Context provider must be a managed Context object");
        }
        entity_registry_.emplace(std::allocator_arg,
                                 Allocator(memory_resource()),
                                 registry,
                                 *registered.owner,
                                 registered.slot);
    }

    template <class Owner, class T>
        requires std::derived_from<T, EntityRegistry>
    UserContext& with_entity_registry(Owner& origin, T& registry) {
        set_entity_registry(origin, registry);
        return *this;
    }

    const EntityDescriptor* entity(std::string_view name) const {
        ensure_open();
        if (metadata_borrow_) {
            (void)metadata_borrow_->get();
        }
        Arena::Quiescence active;
        if (!memory_.closing()) {
            active.attach(const_cast<detail::ContextArena&>(memory_), nullptr, [](void*) noexcept {
                std::terminate();
            });
        }
        return metadata_ ? metadata_->entity(name) : nullptr;
    }

    Vector<const EntityDescriptor*> all_entities() const {
        ensure_open();
        if (metadata_borrow_) {
            (void)metadata_borrow_->get();
        }
        Arena::Quiescence active;
        if (!memory_.closing()) {
            active.attach(const_cast<detail::ContextArena&>(memory_), nullptr, [](void*) noexcept {
                std::terminate();
            });
        }
        auto r = const_cast<detail::ContextArena*>(&memory_);
        return metadata_ ? metadata_->all_entities(r) : Vector<const EntityDescriptor*>(r);
    }

    Result<ResourceBorrow<const EntityDescriptor>, RuntimeError>
    require_entity(std::string_view name) const {
        using R = Result<ResourceBorrow<const EntityDescriptor>, RuntimeError>;
        auto* found = entity(name);
        auto r = const_cast<detail::ContextArena*>(&memory_);
        if (!found) {
            return R::err(r, RuntimeError::missing_entity(r, name));
        }
        return R::ok(r,
                     ResourceBorrow<const EntityDescriptor>(
                         std::allocator_arg,
                         Allocator(r),
                         *found,
                         detail::resource_arena(found->memory_resource())));
    }

    bool has_entity_data_service(std::string_view name) const {
        ensure_open();
        if (entity_registry_) {
            const auto& registry = entity_registry_->get();
            Arena::Quiescence active;
            if (!memory_.closing()) {
                active.attach(
                    const_cast<detail::ContextArena&>(memory_), nullptr, [](void*) noexcept {
                        std::terminate();
                    });
            }
            if (registry.contains(name)) {
                return true;
            }
        }
        return entity(name) != nullptr;
    }

    template <class Owner, class T>
        requires std::derived_from<T, runtime::SchemaProvider>
    void set_schema_provider(Owner& origin, T& provider) {
        ensure_mutable();
        check_public();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), provider);
        if (!registered.slot) {
            throw std::invalid_argument("Context provider must be a managed Context object");
        }
        schema_provider_.emplace(std::allocator_arg,
                                 Allocator(memory_resource()),
                                 provider,
                                 *registered.owner,
                                 registered.slot);
    }

    template <class Owner, class T>
        requires std::derived_from<T, runtime::SchemaProvider>
    UserContext& with_schema_provider(Owner& origin, T& provider) {
        set_schema_provider(origin, provider);
        return *this;
    }

    AsyncOperation<RuntimeSchemaResult>& ensure_schema() const;

    template <class Owner, class T>
        requires std::derived_from<T, InternalIdGenerator>
    void set_internal_id_generator(Owner& origin, T& generator) {
        ensure_mutable();
        auto registered =
            detail::ContextResources::entry(memory_, origin.memory_resource(), generator);
        internal_id_generator_.emplace(std::allocator_arg,
                                       Allocator(memory_resource()),
                                       generator,
                                       *registered.owner,
                                       registered.slot);
    }

    template <class T>
        requires std::derived_from<T, InternalIdGenerator>
    void set_internal_id_generator(Ref<T> reference) {
        set_checked_resource(internal_id_generator_, reference);
    }

    template <class Owner, class T>
        requires std::derived_from<T, InternalIdGenerator>
    UserContext& with_internal_id_generator(Owner& origin, T& generator) {
        set_internal_id_generator(origin, generator);
        return *this;
    }

    template <class T>
        requires std::derived_from<T, InternalIdGenerator>
    UserContext& with_internal_id_generator(Ref<T> reference) & {
        set_internal_id_generator(reference);
        return *this;
    }

    Result<std::optional<std::uint64_t>, RuntimeError> generate_id(std::string_view entity) const {
        ensure_open();
        auto r = const_cast<detail::ContextArena*>(&memory_);
        using R = Result<std::optional<std::uint64_t>, RuntimeError>;
        if (!internal_id_generator_) {
            return R::ok(r, {});
        }
        Arena::Quiescence active;
        active.attach(*r, nullptr, [](void*) noexcept {
            std::terminate();
        });
        auto id = internal_id_generator_->get().generate_id(r, entity);
        if (!id) {
            return R::err(r, std::move(id).error());
        }
        return R::ok(r, id.value());
    }

    Result<std::uint64_t, RuntimeError> next_id(std::string_view entity) const {
        auto generated = generate_id(entity);
        auto r = const_cast<detail::ContextArena*>(&memory_);
        using R = Result<std::uint64_t, RuntimeError>;
        if (!generated) {
            return R::err(r, std::move(generated).error());
        }
        if (generated.value()) {
            return R::ok(r, *generated.value());
        }
        return global_.local_id_generator().generate_id(r, entity);
    }

    template <Entity E>
    Result<std::monostate, RuntimeError> initialize_generated_bootstrap_entity(
        E& entity, std::string_view entity_name, std::uint64_t fixed_id) const {
        ensure_open();
        auto r = const_cast<detail::ContextArena*>(&memory_);
        using R = Result<std::monostate, RuntimeError>;
        if (!internal_id_generator_) {
            return R::err(r, RuntimeError::id_generation(r, "missing internal ID generator"));
        }
        Arena::Quiescence active;
        active.attach(*r, nullptr, [](void*) noexcept {
            std::terminate();
        });
        auto reserved = internal_id_generator_->get().ensure_floor(r, entity_name, fixed_id);
        if (!reserved) {
            return reserved;
        }
        entity.mark_as_new();
        return R::ok(r, {});
    }

private:
    friend class RuntimeModule;
    friend class ContextBinding;
    friend Arena& detail::checker_reference_origin(const UserContext&, std::uint64_t);
    friend struct detail::ContextEntityAccess;
    const InMemoryEntityGraphDecoderRegistry* graph_decoders_ = nullptr;
    const EntityDataServiceBehaviorRegistry* entity_behaviors_ = nullptr;
    const RequestPolicy* request_policy_ = nullptr;
    std::optional<ResourceBorrow<const RawAuditEventSink>> event_sink_borrow_;
    std::optional<ResourceBorrow<const SafeAuditEventSink>> custom_event_sink_borrow_;
    std::optional<ResourceBorrow<const RequestPolicy>> request_policy_borrow_;
    std::optional<ResourceBorrow<const EntityDataServiceBehaviorRegistry>> entity_behaviors_borrow_;
    friend class detail::GovernanceActivation;
    friend class detail::AuditActivation;
    friend struct detail::ContextAuditAccess;
    Result<std::monostate, RuntimeError> deliver_audit_event(const RawAuditEvent&) const;
    friend class detail::GraphFixActivation;
    friend struct detail::ContextFixAccess;
    friend class detail::SchemaOperation;
    friend bool detail::is_generated_schema_bootstrap(const UserContext&);
    mutable std::size_t pending_schema_operations_ = 0;
    std::optional<ResourceBorrow<const runtime::SchemaProvider>> schema_provider_;
    const Vector<GeneratedSchemaBootstrap>* generated_schema_bootstraps_ = nullptr;

    void ensure_mutable() const {
        ensure_open();
        if (memory_.closing()) {
            throw std::logic_error("Context is closing");
        }
        if (pending_schema_operations_) {
            throw std::logic_error("Context is borrowed by schema initialization");
        }
    }

    std::optional<ResourceBorrow<const BusinessClock>> business_clock_;
    MetadataStore* metadata_ = nullptr;
    std::optional<ResourceBorrow<const EntityRegistry>> entity_registry_;
    std::optional<ResourceBorrow<const InternalIdGenerator>> internal_id_generator_;
    GlobalContext& global_;
    detail::ContextArena memory_;
    std::uint32_t identity_ = 0;
    detail::ContextState* state_ = nullptr;
    mutable StringStorage* strings_ = nullptr;
    mutable EntityRuntimeState* entity_runtime_state_ = nullptr;
    Language language_ = Language::English;
    std::optional<ResourceBorrow<const I18nCatalog>> i18n_catalog_;
    std::optional<ResourceBorrow<const CheckerRegistry>> checker_registry_;
    std::optional<ResourceBorrow<const MetadataStore>> metadata_borrow_;
    mutable Vector<FixEvidence>* last_fix_evidence_ = nullptr;
    String* trace_id_ = nullptr;
    std::optional<ResourceBorrow<const MutationPolicyRegistry>> mutation_policy_registry_;
    std::optional<ResourceBorrow<const MutationPolicyApprovalProvider>>
        mutation_policy_approval_provider_;
    std::optional<ResourceBorrow<const MutationGovernanceSink>> mutation_governance_sink_;
    mutable FieldSet* emitted_mutation_governance_warnings_ = nullptr;
    void emit_mutation_governance_warning(const MutationGovernanceSnapshot&,
                                          std::string_view) const;

    template <class Interface, class Owner, class T>
    void set_checked_resource(std::optional<ResourceBorrow<const Interface>>& destination,
                              Owner& origin,
                              T& value) {
        check_public();
        ensure_mutable();
        auto registered = detail::checker_resource(memory_, origin, value);
        destination.emplace(std::allocator_arg,
                            Allocator(memory_resource()),
                            value,
                            *registered.owner,
                            registered.slot);
    }

    template <class Interface, class T>
    void set_checked_resource(std::optional<ResourceBorrow<const Interface>>& destination,
                              Ref<T> reference) {
        auto& origin = reference_arena(reference.id(), std::source_location::current());
        ensure_mutable();
        auto& value = get(reference);
        (void)detail::ContextResources::entry(
            memory_, &origin, value, detail::reference_slot(reference.id()));
        destination.emplace(std::allocator_arg,
                            Allocator(memory_resource()),
                            value,
                            origin,
                            detail::reference_slot(reference.id()));
    }

    void replace_last_fix_evidence(std::span<const FixEvidence>) const;
    RuntimeHookResult validate_required_create_payload(std::string_view entity,
                                                       const EntityValues&,
                                                       const ObjectLocation&) const;

    void collect_required_property_results(std::string_view,
                                           const EntityValues&,
                                           CheckObjectStatus,
                                           const ObjectLocation&,
                                           CheckResults&) const;

    std::uint64_t scope_id(bool global, std::uint32_t slot = 0) const noexcept {
        return detail::reference_id(global_.runtime_identity_, identity_, global, slot);
    }

    void check_public(std::uint64_t id = 0,
                      std::source_location access = std::source_location::current()) const {
        memory_.check_thread();
        if (state_->phase != detail::ContextState::Phase::Open) {
            throw ReferenceError(ReferenceError::Kind::Closed, id, access, state_->closed_at);
        }
    }

    Arena& reference_arena(std::uint64_t id, std::source_location access) const {
        check_public(id, access);
        if (id == 0) {
            throw ReferenceError(ReferenceError::Kind::Invalid, id, access);
        }
        if (detail::reference_runtime(id) != global_.runtime_identity_) {
            throw ReferenceError(ReferenceError::Kind::ForeignRuntime, id, access);
        }
        auto context = detail::reference_context(id);
        auto* state = global_.directory_->find(context);
        if (!state) {
            throw ReferenceError(ReferenceError::Kind::Invalid, id, access);
        }
        if (state->phase != detail::ContextState::Phase::Open) {
            throw ReferenceError(ReferenceError::Kind::Closed, id, access, state->closed_at);
        }
        if (context != identity_) {
            throw ReferenceError(ReferenceError::Kind::ForeignContext, id, access);
        }
        return detail::reference_global(id) ? static_cast<Arena&>(global_.memory_)
                                            : const_cast<detail::ContextArena&>(memory_);
    }

    void check_global_rebind(std::uint64_t id, std::source_location access) const {
        check_public(id, access);
        if (id == 0) {
            throw ReferenceError(ReferenceError::Kind::Invalid, id, access);
        }
        if (detail::reference_runtime(id) != global_.runtime_identity_) {
            throw ReferenceError(ReferenceError::Kind::ForeignRuntime, id, access);
        }
        if (!detail::reference_global(id)) {
            throw ReferenceError(ReferenceError::Kind::NotGlobal, id, access);
        }
        auto origin = detail::reference_context(id);
        if (origin != 0) {
            auto* state = global_.directory_->find(origin);
            if (!state || !state->published) {
                throw ReferenceError(ReferenceError::Kind::Invalid, id, access);
            }
        }
    }

    StringStorage& strings() const {
        if (!strings_) {
            strings_ = &const_cast<detail::ContextArena&>(memory_).make_payload<StringStorage>(
                memory_.limits().strings);
        }
        return *strings_;
    }

    BytesRef save_bytes_in(std::span<const std::byte> bytes, bool global) const {
        check_public();
        auto* arena = global ? static_cast<Arena*>(&global_.memory_)
                             : const_cast<detail::ContextArena*>(&memory_);
        if (bytes.empty()) {
            return BytesRef(scope_id(global), {});
        }
        auto* storage = static_cast<std::byte*>(arena->allocate(bytes.size(), alignof(std::byte)));
        std::memcpy(storage, bytes.data(), bytes.size());
        return BytesRef(scope_id(global), {storage, bytes.size()});
    }

    void ensure_open() const {
        if (memory_.closed()) {
            throw std::logic_error("Context is closed");
        }
    }

    detail::ContextResources& resources() {
        ensure_mutable();
        if (!resources_) {
            resources_ = &memory_.make<detail::ContextResources>();
        }
        return *resources_;
    }

    template <class T>
    Result<ResourceBorrow<T>, ContextError> required(const detail::RegisteredResource* entry,
                                                     ContextError::Kind missing,
                                                     std::string_view name) const {
        auto* r = const_cast<detail::ContextArena*>(&memory_);
        using Output = Result<ResourceBorrow<T>, ContextError>;
        if (!entry) {
            return Output::err(r, ContextError(std::allocator_arg, Allocator(r), missing, name));
        }
        return Output::ok(r,
                          ResourceBorrow<T>(std::allocator_arg,
                                            Allocator(r),
                                            *static_cast<T*>(entry->object),
                                            *entry->owner,
                                            entry->slot));
    }

    detail::ContextResources* resources_ = nullptr;
    friend struct detail::ContextPaginationAccess;
    friend struct detail::ContextLockAccess;
    SystemTime remote_lock_created_at_ = SystemTime::now();
    mutable String* remote_lock_owner_ = nullptr;
    mutable ContinuousPageCursorStore* continuous_page_cursor_store_ = nullptr;
    mutable IdSetStore* id_set_store_ = nullptr;
    mutable std::optional<ResourceBorrow<ContinuousPageCursorStore>> continuous_page_cursor_borrow_;
    mutable std::optional<ResourceBorrow<IdSetStore>> id_set_store_borrow_;
    struct PaginationObservation;
    mutable PaginationObservation* pagination_observation_ = nullptr;
    mutable std::optional<ResourceBorrow<const RuntimeTelemetry>> runtime_telemetry_;
    const RawAuditEventSink* event_sink_ = nullptr;
    const SafeAuditEventSink* custom_event_sink_ = nullptr;
    const Vector<GraphNode>* initial_graphs_ = nullptr;
    const Vector<GraphNode>* root_graphs_ = nullptr;
    mutable const ContextEntityRef* active_root_ = nullptr;
    String* user_identifier_ = nullptr;
    SqlLogOptions sql_log_options_;
    String* timezone_ = nullptr;
    mutable Vector<SqlLogEntry>* sql_log_entries_ = nullptr;
};

inline std::unique_ptr<UserContext>
GlobalContext::make_user_context(std::size_t initial_block_size) {
    return std::make_unique<UserContext>(*this, initial_block_size);
}

inline std::unique_ptr<UserContext>
GlobalContext::make_user_context(MemoryLimits limits, std::size_t initial_block_size) {
    return std::make_unique<UserContext>(*this, limits, initial_block_size);
}
}
