// Semantic port of both locked runtime business-clock integration tests.
#include <teaql/runtime/context.hpp>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void context_owned_clock_is_deterministic() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto expected = *UtcDateTime::from_ymd_hms_nano(2032, 2, 29, 10, 15, 30, 0);
    auto& clock = user->make<FixedBusinessClock>(expected);
    user->set_business_clock(*user, clock);
    check(user->business_time() == expected && user->business_date() == expected.date_naive(),
          "Context-owned clock preserves the original leap-date fixture");
}

static void existing_business_date_resource_remains_a_compatible_override() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto expected = *UtcDateTime::from_ymd_hms_nano(2032, 2, 29, 10, 15, 30, 0);
    auto& clock = user->make<FixedBusinessClock>(expected);
    user->set_business_clock(*user, clock);
    auto& date = user->make<BusinessDate>(*Date::from_ymd(2040, 1, 2));
    user->insert_resource(*user, date);
    check(user->business_time() == expected && user->business_date() == date.value,
          "date resource overrides only the date, not the deterministic time");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    context_owned_clock_is_deterministic();
    existing_business_date_resource_remains_a_compatible_override();
}
