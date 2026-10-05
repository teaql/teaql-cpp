#include <teaql/runtime/context.hpp>
#include <teaql/provider_sqlite/business_id.hpp>
#include <iostream>
#include <thread>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejected(F&& f, const char* message) {
    bool failed = false;
    try {
        f();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

struct Slot {
    using allocator_type = Allocator;
    std::optional<BusinessIdValue> value;
    bool fresh = true;
    Memory resource;

    Slot(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    std::optional<std::string_view> current_business_id() const {
        return value ? std::optional<std::string_view>(value->value) : std::nullopt;
    }

    bool is_new_aggregate() const {
        return fresh;
    }

    void assign_business_id(BusinessIdValue v) {
        value.emplace(std::allocator_arg, Allocator(resource), v);
    }
};

static BusinessIdPlan plan(Memory r,
                           std::string_view root = "root",
                           std::uint64_t initial = 0,
                           std::uint64_t maximum = 100) {
    BusinessIdScope scope(std::allocator_arg, Allocator(r), root, "Order", "orders", "20261003");
    return BusinessIdPlan(
        std::allocator_arg, Allocator(r), scope, "ORD", std::nullopt, 6, "-", initial, maximum);
}

static void clocks_and_context() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto leap = *UtcDateTime::from_timestamp(-1, 1500000000);
    auto& clock = user->make<FixedBusinessClock>(leap);
    user->set_business_clock(*user, clock);
    auto& retained = global.make<FixedBusinessClock>(clock);
    check(user->business_time() == leap && user->business_date() == *Date::from_ymd(1969, 12, 31),
          "business clock preserves pre-epoch fractional leap seconds");
    auto& override = user->make<BusinessDate>(*Date::from_ymd(2026, 10, 3));
    user->insert_resource(*user, override);
    check(user->business_date() == override.value && user->business_time() == leap,
          "BusinessDate changes date without changing clock time");
    auto sibling = global.make_user_context();
    rejected(
        [&] {
            sibling->set_business_clock(*user, clock);
        },
        "sibling cannot borrow a shorter Context clock");
    bool readable = false, replacement_rejected = false;

    struct Closing {
        UserContext& context;
        FixedBusinessClock& clock;
        bool& readable;
        bool& rejected;
    };

    Closing data{*user, clock, readable, replacement_rejected};
    Arena::Quiescence stop;
    stop.attach(*dynamic_cast<Arena*>(r), &data, [](void* raw) noexcept {
        auto& e = *static_cast<Closing*>(raw);
        e.readable = e.context.business_time().timestamp() == -1;
        try {
            e.context.set_business_clock(e.context, e.clock);
        } catch (const std::logic_error&) {
            e.rejected = true;
        }
    });
    user.reset();
    check(readable && replacement_rejected && retained.business_time() == leap,
          "clock remains readable through quiescence and explicit copy survives close");
    check(!UtcDateTime::from_timestamp(0, 1000000000) &&
              !UtcDateTime::from_timestamp(59, 2000000000) &&
              !UtcDateTime::from_timestamp(INT64_MIN, 0),
          "invalid nanos and full-range timestamps fail without overflow");
    auto now = UtcDateTime::now();
    auto observed = sibling->business_time();
    auto after = UtcDateTime::now();
    check(observed.timestamp() >= now.timestamp() && observed.timestamp() <= after.timestamp(),
          "default clock uses current UTC system time");
    sibling->close();
    rejected(
        [&] {
            sibling->business_date();
        },
        "closed Context refuses clock/resource access");
}

static void services_and_domains() {
    GlobalContext global;
    auto& allocator = global.make<InMemoryBusinessIdAllocator>();
    auto& service = global.make<BusinessIdService>(global, allocator);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& fixed =
        global.make<FixedBusinessClock>(*UtcDateTime::from_ymd_hms_nano(2026, 10, 3, 0, 0, 0, 123));
    user->set_business_clock(global, fixed);
    auto key = BusinessIdEncodingKey::create(r, 1, {}).value();
    auto& local_key = user->make<StaticBusinessIdKeyProvider>(key);
    auto& missing = user->make<Result<std::monostate, ContextError>>(
        user->set_business_id_key_provider(*user, local_key));
    check(!missing && missing.error().kind() == ContextError::Kind::MissingTypedResource,
          "key provider registration requires an existing BusinessIdService");
    user->set_business_id_service(global, service);
    check(bool(user->set_business_id_key_provider(*user, local_key)),
          "Context replaces a service copy when configuring the key provider");
    auto& d = user->make<BusinessIdDefinition>("id", "ORD", "orders");
    auto& slot = user->make<Slot>();
    check(service.ensure(r, *user, d, "root", "Order", slot).error().code ==
              BusinessIdErrorCode::KeyNotFound,
          "Context key configuration does not mutate an independently retained service");
    auto& result = user->make<Result<BusinessIdValue, BusinessIdError>>(
        user->business_ids().value().get().ensure(r, *user, d, "root", "Order", slot));
    check(bool(result) && slot.value && slot.value->value == result.value().value,
          "service assigns only a successful formatted ID");
    rejected(
        [&] {
            global.make<BusinessIdService>(user->business_ids().value().get());
        },
        "service clone cannot retain its User-owned key provider in Global");
    auto& global_key = global.make<StaticBusinessIdKeyProvider>(key);
    check(bool(user->set_business_id_key_provider(global, global_key)),
          "global key provider can be borrowed by a User service");
    auto& retained = global.make<BusinessIdService>(user->business_ids().value().get());
    auto& kept = global.make<BusinessIdValue>(result.value());
    auto& local_allocator = user->make<InMemoryBusinessIdAllocator>();
    auto& local_service = user->make<BusinessIdService>(*user, local_allocator);
    rejected(
        [&] {
            global.make<BusinessIdService>(local_service);
        },
        "service clone cannot extend its User-owned allocator");
    {
        auto again = user->business_ids().value().get().ensure(r, *user, d, "root", "Order", slot);
        check(again.value().value == kept.value,
              "existing ID validation consumes no extra sequence");
    }
    auto next = global.make_user_context();
    next->set_business_clock(global, fixed);
    auto n = next->memory_resource();
    auto& ndef = next->make<BusinessIdDefinition>(d);
    user.reset();
    auto& fresh = next->make<Slot>();
    auto next_result = retained.ensure(n, *next, ndef, "root", "Order", fresh);
    check(next_result && next_result.value().value != kept.value,
          "service clone shares allocation state while retaining explicit Global lifetime");
    auto& legacy = next->make<BusinessIdDefinition>(
        BusinessIdDefinition::legacy_daily_sequence(n, "legacy_id", "OLD", "legacy"));
    auto& old = next->make<Slot>();
    old.fresh = false;
    check(retained.ensure(n, *next, legacy, "root", "Order", old).error().code ==
                  BusinessIdErrorCode::Immutable &&
              !old.value,
          "persisted empty slots fail before allocation");
}

struct FailingResource : std::pmr::memory_resource {
    bool fail = false;

    void* do_allocate(std::size_t n, std::size_t a) override {
        if (fail) {
            throw std::bad_alloc{};
        }
        return std::pmr::new_delete_resource()->allocate(n, a);
    }

    void do_deallocate(void* p, std::size_t n, std::size_t a) override {
        std::pmr::new_delete_resource()->deallocate(p, n, a);
    }

    bool do_is_equal(const std::pmr::memory_resource& r) const noexcept override {
        return this == &r;
    }
};

static void allocation_edges() {
    FailingResource upstream;
    GlobalContext owner(&upstream, 64);
    auto& allocator = owner.make<InMemoryBusinessIdAllocator>();
    GlobalContext results;
    auto user = results.make_user_context();
    auto r = user->memory_resource();
    String root(20000, 'x', r);
    auto p = plan(r, root);
    upstream.fail = true;
    bool failed = false;
    try {
        allocator.allocate(r, p);
    } catch (const std::bad_alloc&) {
        failed = true;
    }
    upstream.fail = false;
    check(failed, "fixture fails while mutating allocator-owned state");
    auto poisoned = allocator.allocate(r, p);
    check(!poisoned && poisoned.error().code == BusinessIdErrorCode::Allocation &&
              poisoned.error().message == "in-memory business ID allocator lock is poisoned",
          "unwinding the critical section preserves the source poisoned state");
    check(allocator.allocate(r, plan(r, "bad", 2, 1)).error().code ==
              BusinessIdErrorCode::InvalidDefinition,
          "range validation occurs before poisoned lock reporting");
    auto& clean = results.make<InMemoryBusinessIdAllocator>();
    check(clean.allocate(r, plan(r, "overflow", UINT64_MAX, UINT64_MAX)).error().code ==
              BusinessIdErrorCode::Exhausted,
          "u64 overflow does not wrap allocator state");
    check(clean.allocate(r, plan(r, "overflow", 0, 5)).value().sequence == 0,
          "overflow failure leaves missing scope uninstalled");
    bool rejected_thread = false;
    std::thread worker([&] {
        try {
            clean.allocate(r, p);
        } catch (const std::logic_error&) {
            rejected_thread = true;
        }
    });
    worker.join();
    check(rejected_thread,
          "Context allocator rejects worker-thread mutation before touching arena memory");
}

static void sqlite_infrastructure() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& executor = global.make<SqliteMutationExecutor>(connection);
    auto& allocator = global.make<SqliteBusinessIdAllocator>(executor);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_business_id_infrastructure(global, allocator);
    auto& clock = user->make<FixedBusinessClock>(*UtcDateTime::from_timestamp(0, 0));
    user->set_business_clock(*user, clock);
    auto& d = user->make<BusinessIdDefinition>(
        BusinessIdDefinition::legacy_daily_sequence(r, "id", "ORD", "orders"));
    auto& slot = user->make<Slot>();
    auto& service = user->business_ids().value().get();
    check(service.ensure(r, *user, d, "root", "Order", slot).error().code ==
                  BusinessIdErrorCode::Allocation &&
              !slot.value,
          "installing infrastructure performs no implicit DDL");
    auto* schema = user->get_resource<detail::BusinessIdSchemaService>();
    check(schema && schema->ensure_schema(r, *user),
          "infrastructure installs the independent schema service");
    check(
        service.ensure(r, *user, d, "root", "Order", slot).value().value == "ORD-19700101-00000001",
        "SQLite infrastructure serves real generated business IDs after explicit schema installation");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        clocks_and_context();
        services_and_domains();
        allocation_edges();
        sqlite_infrastructure();
        std::pmr::set_default_resource(previous);
        std::cout << "Business clock/service contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
