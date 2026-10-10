#include <teaql/core/chrono_value.hpp>
#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void native_utc_conversion() {
    using namespace std::chrono;
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto negative = value_from_utc(owner, sys_time<nanoseconds>(nanoseconds(-1)));
    auto exported = core::value_from_utc(owner, sys_time<milliseconds>(milliseconds(42)));
    check(exported.try_timestamp() == Timestamp(42),
          "core crate facade exposes the native conversion factory");
    check(negative.try_timestamp() == Timestamp(-1),
          "negative fractional milliseconds round down rather than truncate toward zero");
    check(value_from_utc(owner, sys_time<microseconds>(microseconds(-1001))).try_timestamp() ==
              Timestamp(-2),
          "microsecond conversion preserves the same negative boundary");
    check(value_from_utc(owner, sys_time<seconds>(seconds(-1))).try_timestamp() == Timestamp(-1000),
          "seconds scale exactly into signed milliseconds");
    check(value_from_utc(owner, sys_time<milliseconds>(milliseconds(INT64_MIN))).try_timestamp() ==
              Timestamp(INT64_MIN),
          "native millisecond input preserves the complete scalar range");
    bool overflow = false;
    try {
        (void)value_from_utc(owner, sys_time<seconds>(seconds(INT64_MAX)));
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    check(overflow, "native seconds overflow is rejected before multiplication");
    auto native = value_from_utc(owner, sys_time<nanoseconds>(nanoseconds(INT64_MIN)));
    auto& retained = global.make<Value>(native);
    user.reset();
    check(retained.try_timestamp() == Timestamp(-9223372036855LL) &&
              retained.memory_resource() == global.memory_resource(),
          "explicit destination retention survives actual source User destruction");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        native_utc_conversion();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
