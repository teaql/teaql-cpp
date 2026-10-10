#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <chrono>
#include <iostream>
#include <type_traits>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(std::is_trivially_copyable_v<Timestamp>);
static_assert(std::totally_ordered<Timestamp>);
static_assert(std::same_as<decltype((std::declval<Timestamp&>().millis)), std::int64_t&>);
static_assert(sizeof(Timestamp) == sizeof(std::int64_t));

static void public_field_contracts() {
    Timestamp timestamp(2000);
    const Timestamp original = timestamp;
    auto& millis = timestamp.millis;
    millis = -1001;
    check(timestamp.as_millis() == -1001 && original.as_millis() == 2000,
          "public field mutation updates only the selected scalar");
    check(timestamp.to_datetime() == DateTime{*Date::from_ymd(1969, 12, 31), 23, 59, 58, 999},
          "datetime observes mutable public milliseconds with negative floor");
    check(timestamp < original &&
              std::hash<Timestamp>{}(timestamp) == std::hash<std::int64_t>{}(-1001),
          "ordering and hashing observe the same public field");
    GlobalContext global;
    auto user = global.make_user_context();
    check(timestamp.to_json_value(user->memory_resource()).dump(user->memory_resource()) ==
                  "-1001" &&
              debug_string(user->memory_resource(), timestamp) == "Timestamp(-1001)",
          "transparent JSON and Debug observe field mutation");
}

// Exact inputs and assertions from the two original time.rs tests.
static void original_timestamp_contracts() {
    auto timestamp = Timestamp(1000);
    check(timestamp.as_millis() == 1000, "original signed timestamp conversion");
    check(timestamp.to_datetime() == DateTime{*Date::from_ymd(1970, 1, 1), 0, 0, 1, 0},
          "original one-second datetime conversion");
    check(Timestamp::from_u64(2000).as_millis() == 2000, "original unsigned timestamp conversion");
    auto millis_now = [] {
        return std::chrono::floor<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    };
    auto before = millis_now();
    auto now = Timestamp::now();
    auto after = millis_now();
    check(before <= now.as_millis() && now.as_millis() <= after,
          "original current timestamp lies between system-clock observations");
}

static void timestamp_ownership_contracts() {
    GlobalContext global;
    Ref<Timestamp> retained;
    Ref<Json> wire;
    Ref<String> diagnostic;
    {
        auto user = global.make_user_context();
        auto timestamp = user->make_ref<Timestamp>(INT64_MIN);
        retained = user->make_global<Timestamp>(user->get(timestamp));
        wire = user->make_global<Json>(user->get(timestamp).to_json_value(user->memory_resource()));
        diagnostic =
            user->make_global<String>(debug_string(user->memory_resource(), user->get(timestamp)));
        check(Timestamp::from_u64(UINT64_MAX) == Timestamp(-1),
              "unsigned conversion preserves source wrapping");
    }
    check(global.live_user_bytes() == 0, "actual User destruction returns all arena blocks");
    auto next = global.make_user_context();
    auto timestamp = next->get(next->rebind_global(retained));
    const auto& json = next->get(next->rebind_global(wire));
    const auto& text = next->get(next->rebind_global(diagnostic));
    check(timestamp == Timestamp(INT64_MIN) && Timestamp::from_json_value(json) == timestamp,
          "retained transparent wire and copied scalar survive actual User deletion");
    check(text == "Timestamp(-9223372036854775808)" &&
              text.get_allocator().resource() == global.memory_resource(),
          "complete retained raw diagnostic uses Global character storage");
    check(timestamp.to_datetime() == DateTime{},
          "out-of-chrono-range timestamps preserve source epoch fallback");
    String formatted(next->memory_resource());
    core::DebugWriter writer(formatted, {});
    core::write_debug(writer, Timestamp(-1));
    check(formatted == "Timestamp(-1)", "qualified public facade exposes Timestamp Debug");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        original_timestamp_contracts();
        public_field_contracts();
        timestamp_ownership_contracts();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
