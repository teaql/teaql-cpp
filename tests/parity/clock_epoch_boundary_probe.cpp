#include <teaql/core/time.hpp>
#include <teaql/runtime/business_clock.hpp>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::invalid_argument("Expected a clock boundary mode");
        }
        auto mode = std::string_view(argv[1]);
        if (mode == "timestamp") {
            std::cout << teaql::Timestamp::now().as_millis() << '\n';
        } else if (mode == "utc") {
            auto value = teaql::UtcDateTime::now();
            std::cout << value.timestamp() * 1000 + value.timestamp_subsec_nanos() / 1000000
                      << '\n';
        } else if (mode == "negative_value") {
            auto value = teaql::UtcDateTime::from_timestamp(-1, 999000000).value();
            std::cout << teaql::Timestamp(-1).as_millis() << ','
                      << value.timestamp() * 1000 + value.timestamp_subsec_nanos() / 1000000
                      << '\n';
        } else {
            throw std::invalid_argument("Unknown clock boundary mode");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
