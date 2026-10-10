#include <teaql/data_service/model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void arithmetic_and_adapters() {
    auto low = SystemTime::min();
    auto high = SystemTime::max();
    auto span = high.duration_since(low);
    check(
        span && span->seconds == UINT64_MAX && span->nanos == 999999999 &&
            !low.duration_since(high),
        "the entire signed Unix interval retains unsigned seconds and nanoseconds without overflow");
    auto borrowed =
        SystemTime::from_unix_seconds(1, 1).duration_since(SystemTime::from_unix_seconds(0, 2));
    check(borrowed && borrowed->seconds == 0 && borrowed->nanos == 999999999,
          "duration subtraction borrows one second across fractional boundaries");
    check(low.checked_add_seconds(UINT64_MAX)->unix_seconds() == INT64_MAX &&
              high.checked_sub_seconds(UINT64_MAX)->unix_seconds() == INT64_MIN &&
              !high.checked_add_seconds(1) && !low.checked_sub_seconds(1),
          "checked deadline arithmetic spans both signed boundaries and rejects overflow");
    for (auto ticks : {INT64_MIN, INT64_C(-1), INT64_C(0), INT64_C(1), INT64_MAX}) {
        SystemTime direct(std::chrono::nanoseconds{ticks});
        SystemTime adapted(std::chrono::system_clock::time_point{std::chrono::nanoseconds{ticks}});
        check(
            direct == adapted && direct.time_since_epoch().count() == ticks,
            "both native chrono adapters round-trip all nanosecond endpoints without signed overflow");
    }
    for (auto time : {low, high, SystemTime::from_unix_seconds(10000000000)}) {
        bool rejected = false;
        try {
            (void)time.time_since_epoch();
        } catch (const std::overflow_error&) {
            rejected = true;
        }
        check(rejected,
              "out-of-chrono values remain valid times and reject only the narrowing adapter");
    }
    bool invalid = false;
    try {
        (void)SystemTime::from_unix_seconds(0, 1000000000);
    } catch (const std::invalid_argument&) {
        invalid = true;
    }
    check(invalid, "SystemTime storage requires normalized nanoseconds");
    auto before = SystemTime::now();
    auto now = SystemTime::clock::now();
    auto after = SystemTime::now();
    check(before <= now && now <= after,
          "provider clock reads remain ordered inside an independent bracket");
}

static void retained_metadata() {
    GlobalContext global;
    Ref<ExecutionMetadata> retained;
    Ref<String> expected;
    {
        auto user = global.make_user_context();
        auto& metadata = user->make<ExecutionMetadata>(
            DataServiceOperation::Query, SystemTime::min(), SystemTime::max());
        auto text = debug_string(user->memory_resource(), metadata);
        check(
            text.find("tv_sec: -9223372036854775808") != String::npos &&
                text.find("tv_sec: 9223372036854775807") != String::npos,
            "execution metadata preserves complete Unix time boundaries in its public fields and diagnostics");
        retained = user->make_global<ExecutionMetadata>(metadata);
        expected = user->make_global<String>(text);
    }
    check(global.live_user_bytes() == 0, "wide scalar times retain no source User blocks");
    auto user = global.make_user_context();
    const auto& metadata = user->get(user->rebind_global(retained));
    check(
        metadata.started_at == SystemTime::min() && metadata.ended_at == SystemTime::max() &&
            debug_string(user->memory_resource(), metadata) ==
                user->get(user->rebind_global(expected)),
        "wide time metadata and diagnostics survive explicit destination copying and actual User deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        arithmetic_and_adapters();
        retained_metadata();
        std::pmr::set_default_resource(previous);
        std::cout << "SystemTime contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
