#include <teaql/data_service.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include <type_traits>
#include <unordered_map>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(std::is_trivially_copyable_v<SystemTime>);
static_assert(std::is_trivially_copyable_v<SystemTimeError>);
static_assert(SystemTime::UNIX_EPOCH == SystemTime{});
static_assert(UNIX_EPOCH == SystemTime::UNIX_EPOCH);

static void typed_time_errors() {
    auto result = SystemTime::min().duration_since(SystemTime::max());
    check(result.is_err() && result.error().duration() == SystemDuration{UINT64_MAX, 999'999'999},
          "reversed interval preserves the complete positive span in its typed error");
    auto copy = result;
    check(copy.error().duration() == result.error().duration() && copy.error().source() == nullptr,
          "scalar error cloning preserves duration without owning a cause or Context data");
    check(copy.error().message() == "second time provided was later than self",
          "error display preserves the source message");
    DebugOptions rounded;
    rounded.precision = 0;
    auto owner = std::pmr::new_delete_resource();
    check(debug_string(owner, result.error(), rounded) == "SystemTimeError(18446744073709551616s)",
          "duration rounding can display one beyond the unsigned integer bound");
    rounded.precision = 1;
    check(debug_string(owner, SystemDuration{1, 250'000'000}, rounded) == "1.2s" &&
              debug_string(owner, SystemDuration{1, 350'000'000}, rounded) == "1.4s",
          "fractional formatting rounds ties to even");
    rounded.width = 8;
    rounded.alignment = DebugAlignment::Right;
    check(debug_string(owner, SystemDuration{0, 1'500}, rounded) == "   1.5µs",
          "duration padding counts Unicode scalars in the unit");
    auto future = SystemTime::max();
    auto before = SystemTime::now();
    auto elapsed = future.elapsed();
    auto after = SystemTime::now();
    auto span = elapsed.error().duration();
    auto lower = future.duration_since(after).value();
    auto upper = future.duration_since(before).value();
    auto less_equal = [](SystemDuration a, SystemDuration b) {
        return a.seconds < b.seconds || (a.seconds == b.seconds && a.nanos <= b.nanos);
    };
    check(elapsed.is_err() && less_equal(lower, span) && less_equal(span, upper),
          "future elapsed reports the positive duration observed inside the clock bracket");
    bool rejected = false;
    try {
        (void)result.value();
    } catch (const std::bad_variant_access&) {
        rejected = true;
    }
    check(rejected, "error alternatives cannot be read as successful durations");
}

static void arithmetic_and_adapters() {
    std::unordered_map<SystemTime, unsigned> identities;
    identities.emplace(SystemTime::min(), 1);
    identities.emplace(SystemTime::max(), 2);
    identities.emplace(SystemTime::from_unix_seconds(0, 1), 3);
    identities.emplace(SystemTime(std::chrono::nanoseconds(1)), 4);
    check(identities.size() == 3 && identities.at(SystemTime::from_unix_seconds(0, 1)) == 3 &&
              identities.at(SystemTime::min()) == 1 && identities.at(SystemTime::max()) == 2,
          "native hashing preserves full-range keys and equivalent normalized instants");
    auto time = SystemTime::from_unix_seconds(0, 999'999'999);
    time += SystemDuration{0, 1};
    check(time == SystemTime::from_unix_seconds(1), "subsecond carry reaches the next second");
    time = SystemTime{};
    time -= SystemDuration{0, 1};
    check(time == SystemTime::from_unix_seconds(-1, 999'999'999), "pre-epoch borrow is normalized");
    time = SystemTime::min();
    time += SystemDuration{UINT64_MAX, 999'999'999};
    check(time == SystemTime::max(), "full unsigned duration reaches the last source instant");
    time -= SystemDuration{UINT64_MAX, 999'999'999};
    check(time == SystemTime::min(), "full unsigned duration reaches the first source instant");
    for (bool add : {false, true}) {
        auto value = add ? SystemTime::max() : SystemTime::min();
        auto previous = value;
        bool failed = false;
        try {
            if (add) {
                value += SystemDuration{0, 1};
            } else {
                value -= SystemDuration{0, 1};
            }
        } catch (const std::overflow_error&) {
            failed = true;
        }
        check(failed && value == previous, "overflowing assignment preserves its operand");
    }
    time = SystemTime{};
    time += std::chrono::nanoseconds(INT64_MIN);
    check(time.time_since_epoch().count() == INT64_MIN,
          "native negative magnitude does not overflow");
    time -= std::chrono::nanoseconds(INT64_MIN);
    check(time == SystemTime{}, "native negative subtraction restores the epoch");
    check(SystemTime(std::chrono::seconds(INT64_MAX)).unix_seconds() == INT64_MAX,
          "whole seconds do not narrow through a native nanosecond count");
    auto millis = SystemTime(std::chrono::milliseconds(INT64_MIN));
    check(millis.unix_seconds() == -9'223'372'036'854'776 && millis.subsec_nanos() == 192'000'000,
          "millisecond conversion floors without overflowing a nanosecond product");
    auto micros = SystemTime(std::chrono::microseconds(INT64_MAX));
    check(micros.unix_seconds() == 9'223'372'036'854 && micros.subsec_nanos() == 775'807'000,
          "microseconds preserve their complete native count");
    using SecondsPoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
    check(SystemTime(SecondsPoint(std::chrono::seconds(INT64_MIN))).unix_seconds() == INT64_MIN,
          "native clock points convert before nanosecond narrowing");
    bool overflow = false;
    try {
        (void)SystemTime(std::chrono::hours(INT64_MAX));
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    check(overflow, "whole-hour conversion rejects an overflowing seconds product");
}

static void retained_metadata() {
    GlobalContext global;
    Ref<ExecutionMetadata> retained;
    {
        auto user = global.make_user_context();
        auto& metadata = user->make<ExecutionMetadata>(DataServiceOperation::Query);
        metadata.started_at = SystemTime::min();
        metadata.ended_at = SystemTime::max();
        metadata.backend.assign(2048, 'b');
        retained = user->make_global<ExecutionMetadata>(metadata);
    }
    check(global.live_user_bytes() == 0, "actual User deletion returns all source blocks");
    auto next = global.make_user_context();
    const auto& metadata = next->get(next->rebind_global(retained));
    check(metadata.ended_at.duration_since(metadata.started_at) ==
              SystemDuration{UINT64_MAX, 999'999'999},
          "Global metadata retains the full source elapsed span");
    check(metadata.backend.size() == 2048 &&
              metadata.backend.get_allocator().resource() == global.memory_resource() &&
              debug_string(next->memory_resource(), metadata).find("9223372036854775807") !=
                  String::npos,
          "retained wide diagnostics and strings preserve their explicit lifetime");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        arithmetic_and_adapters();
        typed_time_errors();
        retained_metadata();
        std::pmr::set_default_resource(previous);
        std::cout << "System-time arithmetic scope contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
