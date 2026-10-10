#include <teaql/data_service/types.hpp>
#include <teaql/runtime/system_time.hpp>
#include <iostream>

int main() {
    using NativeTime = teaql::ExecutionMetadata::SystemTime;
    static_assert(std::same_as<NativeTime::duration, std::chrono::nanoseconds>);
    std::cout << "Current metadata seconds: " << NativeTime::min().unix_seconds() << ".."
              << NativeTime::max().unix_seconds() << '\n';
    for (auto seconds : {INT64_C(10'000'000'000), -INT64_C(10'000'000'000), INT64_MAX, INT64_MIN}) {
        auto wide = teaql::SystemTime::from_unix_seconds(seconds);
        std::cout << "Existing wide scalar retains " << wide.unix_seconds() << '\n';
    }
}
