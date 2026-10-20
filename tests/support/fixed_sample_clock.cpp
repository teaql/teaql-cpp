// Test-only process clock control; never linked into a TEAQL runtime library.
#include <charconv>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>

extern "C" int clock_gettime(clockid_t clock, timespec* result) noexcept {
    if (clock == CLOCK_REALTIME) {
        if (const char* input = std::getenv("TEAQL_TEST_SAMPLE_WALL_MILLIS")) {
            std::int64_t millis = 0;
            auto [end, error] = std::from_chars(input, input + std::strlen(input), millis);
            if (error == std::errc{} && *end == '\0') {
                auto seconds = millis / 1000;
                auto remainder = millis % 1000;
                if (remainder < 0) {
                    --seconds;
                    remainder += 1000;
                }
                result->tv_sec = static_cast<time_t>(seconds);
                result->tv_nsec = static_cast<long>(remainder * 1000000);
                return 0;
            }
        }
    }
    return static_cast<int>(syscall(SYS_clock_gettime, clock, result));
}

extern "C" int timespec_get(timespec* result, int base) noexcept {
    if (base != TIME_UTC) {
        return 0;
    }
    return clock_gettime(CLOCK_REALTIME, result) == 0 ? TIME_UTC : 0;
}
