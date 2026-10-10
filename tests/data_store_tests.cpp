#include <teaql/core.hpp>
#include <teaql/sql.hpp>
#include <teaql/runtime.hpp>
#include <iostream>
#include <thread>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void context_and_lifetime() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    check(!user->get_in_store("absent").wait(), "missing store reads return None");
    user->put_in_store("absent", Value(r, 9), std::nullopt).wait();
    user->clear_in_store("absent").wait();
    auto stored = user->make_global_resource<InMemoryDataStore>();
    auto& store = user->get(stored);
    user->set_data_store(stored);
    {
        Value::Object payload(r);
        Value::List list(r);
        list.emplace_back(String(4096, 's', r));
        payload.emplace(String("nested", r), Value(r, list));
        String key(128, 'k', r);
        auto& queued = user->put_in_store(key, Value(r, payload), std::nullopt);
        key.assign("changed");
        payload.clear();
        check(!store.get(r, String(128, 'k', r)).wait(), "put is lazy until polling");
        queued.wait();
        auto& saved = store.get(r, String(128, 'k', r)).wait();
        check(saved && saved->memory_resource() == r &&
                  saved->as_object()->begin()->second.as_list()->front().try_text()->size() == 4096,
              "queued keys and nested payloads are snapshots in the output domain");
        user->put_local("local", Value(r, 5));
        check(!user->get_in_store("local").wait(), "store and Context locals stay distinct");
        auto& abandoned = user->put_in_store("cancelled", Value(r, 8), std::nullopt);
        abandoned.cancel();
    }
    user.reset();
    check(global.live_user_bytes() == 0, "Global store retains no User arena blocks");
    auto next = global.make_user_context();
    auto rebound = next->rebind_global(stored);
    auto& retained = next->get(rebound);
    next->set_data_store(rebound);
    auto& copied = next->get_in_store(String(128, 'k', next->memory_resource())).wait();
    check(copied &&
              copied->as_object()->begin()->second.as_list()->front().try_text()->size() == 4096 &&
              copied->as_object()->begin()->second.as_list()->front().memory_resource() ==
                  next->memory_resource(),
          "store values survive actual User deletion and get copies into the next User");
    check(!next->get_in_store("cancelled").wait(), "cancelled put has no side effect");
    auto& pending = next->get_in_store("missing");
    next->destroy(rebound);
    bool rejected = false;
    try {
        pending.wait();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Dead;
    }
    check(rejected, "expired store is rejected before its virtual get method");
}

static void publication_and_ttl() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& store = user->make_resource<InMemoryDataStore>();
    auto& queued = user->get_in_store("late");
    user->insert_resource(*user, static_cast<DataStore&>(store));
    store.put(r, "late", Value(r, "installed after future creation"), std::nullopt).wait();
    check(queued.wait()->try_text() == "installed after future creation",
          "Context resource selection occurs on first poll as in Rust");
    store.put(r, "key", Value(r, 1), 0).wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    check(!store.get(r, "key").wait(), "zero-second expiry is unavailable after time advances");
    store.put(r, "key", Value(r, 2), 60).wait();
    check(store.get(r, "key").wait()->try_i64() == 2, "replacement refreshes the expiry");
    store.remove(r, "key").wait();
    store.remove(r, "key").wait();
    check(!store.get(r, "key").wait(), "remove is idempotent");
    store.put(r, "null", Value(r), std::nullopt).wait();
    check(store.get(r, "null").wait().has_value(), "stored Null remains Some(Null)");
    user->put_in_store("scalar", "converted text", std::nullopt).wait();
    check(user->get_in_store("scalar").wait()->try_text() == "converted text",
          "Context converts scalar input into its explicit Value domain");
    bool overflow = false;
    try {
        store.put(r, "overflow", Value(r, 3), UINT64_MAX).wait();
    } catch (const std::out_of_range&) {
        overflow = true;
    }
    check(overflow, "Instant overflow fails instead of silently wrapping");
    bool poisoned = false;
    try {
        store.get(r, "null").wait();
    } catch (const std::logic_error&) {
        poisoned = true;
    }
    check(poisoned, "source panic under the write lock leaves the store poisoned");
}

struct PendingStore final : DataStore {
    using allocator_type = Allocator;
    Memory resource;
    bool pending = true;
    unsigned cancelled = 0;
    unsigned calls = 0;
    WakeSignal* wake = nullptr;

    PendingStore(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    struct Work {
        PendingStore& store;

        std::optional<std::optional<Value>> poll(WakeSignal& wake) {
            store.wake = &wake;
            if (store.pending) {
                return {};
            }
            return std::optional<std::optional<Value>>(std::in_place,
                                                       Value(store.resource, "ready"));
        }

        void finish_and_wait() noexcept {
            store.wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            store.wake = nullptr;
            ++store.cancelled;
        }
    };

    AsyncOperation<std::optional<Value>>& get(Memory r, std::string_view) const override {
        auto& self = const_cast<PendingStore&>(*this);
        ++self.calls;
        return make_operation<std::optional<Value>, Work>(r, Work{self});
    }

    AsyncOperation<std::monostate>&
    put(Memory, std::string_view, Value, std::optional<std::uint64_t>) const override {
        throw std::logic_error("unexpected put");
    }

    AsyncOperation<std::monostate>& remove(Memory, std::string_view) const override {
        throw std::logic_error("unexpected remove");
    }
};

static void pending_store_composition() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& store = user->make_resource<PendingStore>();
    user->insert_resource(*user, static_cast<DataStore&>(store));
    auto& abandoned = user->get_in_store("abandoned");
    abandoned.cancel();
    check(store.calls == 0, "cancellation before polling does not invoke the store");
    auto& operation = user->get_in_store("pending");
    check(!operation.poll() && store.calls == 1, "pending child retains the outer future");
    auto ticket = operation.wake_signal().ticket();
    store.wake->notify();
    check(operation.wake_signal().ticket() != ticket, "child wake reaches the Context future");
    operation.cancel();
    check(store.cancelled == 1 && !store.wake,
          "cancelling a Context store future detaches and cancels its child exactly once");
    store.pending = false;
    check(user->get_in_store("ready").wait()->try_text() == "ready" && !store.wake,
          "successful completion also detaches the child wake");
}

static void explicit_store_copy() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& store = user->make_resource<InMemoryDataStore>();
    store.put(r, "copy", Value(r, String(4096, 'x', r)), 60).wait();
    auto retained = user->make_global_resource<InMemoryDataStore>(store);
    user.reset();
    auto next = global.make_user_context();
    next->set_data_store(next->rebind_global(retained));
    const auto& value = next->get_in_store("copy").wait();
    check(value && value->try_text()->size() == 4096 &&
              value->memory_resource() == next->memory_resource() &&
              value->try_text()->front() == 'x',
          "explicit Global copying of a User store propagates nested allocator ownership");
}

struct ForeignStore final : DataStore {
    GlobalContext& owner;
    unsigned cancelled = 0;
    bool saw_live_key = false;
    AsyncOperation<std::optional<Value>>* child = nullptr;

    explicit ForeignStore(GlobalContext& owner) : owner(owner) {}

    struct Work {
        ForeignStore& store;
        std::string_view key;

        std::optional<std::optional<Value>> poll(WakeSignal&) {
            return {};
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            ++store.cancelled;
            store.saw_live_key = key == "foreign-domain";
            key = {};
        }
    };

    AsyncOperation<std::optional<Value>>& get(Memory, std::string_view key) const override {
        auto& self = const_cast<ForeignStore&>(*this);
        self.child = &make_operation<std::optional<Value>, Work>(owner, Work{self, key});
        return *self.child;
    }

    AsyncOperation<std::monostate>&
    put(Memory, std::string_view, Value, std::optional<std::uint64_t>) const override {
        throw std::logic_error("unexpected foreign put");
    }

    AsyncOperation<std::monostate>& remove(Memory, std::string_view) const override {
        throw std::logic_error("unexpected foreign remove");
    }
};

static void rejected_foreign_work_is_quiesced() {
    GlobalContext global;
    auto& store = global.make_resource<ForeignStore>(global);
    auto user = global.make_user_context();
    user->insert_resource(global, static_cast<DataStore&>(store));
    auto& operation = user->get_in_store("foreign-domain");
    bool rejected = false;
    try {
        (void)operation.poll();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    const bool quiesced = store.cancelled == 1 && store.saw_live_key &&
                          store.child->status() == AsyncStatus::Cancelled;
    if (!quiesced) {
        // Keep the negative control safe before unwinding its User owner.
        store.child->cancel();
    }
    check(rejected && quiesced,
          "foreign-domain rejection quiesces returned work while its borrowed User key is alive");
    user.reset();
    check(store.cancelled == 1 && global.live_user_bytes() == 0,
          "rejected child cleanup is not deferred until after User reclamation");
}

static_assert(!std::same_as<core::EntityGraphBuilder, runtime::EntityGraphBuilder>);
static_assert(std::same_as<core::Value, Value>);
static_assert(std::same_as<sql::SqlDialect, SqlDialect>);
static_assert(std::same_as<runtime::DataStore, DataStore>);
static_assert(!std::same_as<runtime::SchemaProvider, SchemaProvider>);
static_assert(runtime::CHECK_OBJECT_STATUS_FIELD == "__teaql_object_status");
static_assert(std::same_as<runtime::CheckResults, CheckResults>);
static_assert(std::same_as<runtime::I18nCatalog, I18nCatalog>);

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        context_and_lifetime();
        publication_and_ttl();
        pending_store_composition();
        explicit_store_copy();
        rejected_foreign_work_is_quiesced();
        std::cout << "DataStore public and lifetime contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
