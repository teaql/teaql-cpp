#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

int main() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto reference = user->make_ref<FixedBusinessClock>(*UtcDateTime::from_timestamp(123, 0));
    user->set_business_clock(*user, user->get(reference));
    user->destroy(reference);
    try {
        (void)user->business_time();
    } catch (const ReferenceError& error) {
        std::cout << "Checked rejection: " << unsigned(error.kind()) << '\n';
        return 0;
    }
    std::cerr << "Destroyed clock dispatched without checked rejection\n";
    return 1;
}
