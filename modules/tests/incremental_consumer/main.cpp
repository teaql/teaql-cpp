#include <cstdint>
#include <iostream>
#include <stdexcept>

#if defined(TEAQL_BUILD_USE_MODULES)
import teaql_provider_sqlite;
#else
#include "header_scope.hpp"
#endif

int main() {
    const auto date = teaql::Date::from_ymd(2024, 2, 29);
    if (!date) {
        throw std::runtime_error("Benchmark date must be valid");
    }
    std::cout << date->epoch_days() << ' ' << teaql::Timestamp{}.as_millis() << ' '
              << teaql::Ref<int>{}.id() << ' ' << date->year() << '\n';
}
