#include <functional>
import teaql.assessment.time;

int main() {
    auto time = teaql::Timestamp::from_u64(42);
    auto date = teaql::Date::from_ymd(1970, 1, 1);
    auto hash = std::hash<teaql::Timestamp>{}(time);
    return time.as_millis() == 42 && date && date->epoch_days() == 0 &&
                   hash == std::hash<long>{}(42)
               ? 0
               : 1;
}
