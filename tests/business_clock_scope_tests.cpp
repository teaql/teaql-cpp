#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void dead(F&& call) {
    bool rejected = false;
    try {
        call();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Dead;
    }
    check(rejected, "Destroyed clock rejects before virtual dispatch");
}

struct ClosingClock final : BusinessClock {
    UserContext& context;

    explicit ClosingClock(UserContext& user) : context(user) {}

    UtcDateTime business_time() const override {
        context.close();
        return *UtcDateTime::from_timestamp(123, 0);
    }
};

int main(int argc, char** argv) {
    if (argc == 2) {
        std::set_terminate([] {
            std::_Exit(0);
        });
        GlobalContext global;
        auto user = global.make_user_context();
        auto& clock = user->make_resource<ClosingClock>(*user);
        user->set_business_clock(*user, clock);
        try {
            if (std::string_view(argv[1]) == "reentrant-time") {
                (void)user->business_time();
            } else {
                (void)user->business_date();
            }
        } catch (...) {
            std::_Exit(2);
        }
        return 1;
    }
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto timestamp = *UtcDateTime::from_timestamp(-1, 123456789);
    auto clock = user->make_ref<FixedBusinessClock>(timestamp);
    user->set_business_clock(*user, user->get(clock));
    check(user->business_time() == timestamp && user->business_date() == timestamp.date_naive(),
          "Live checked clock preserves time and inherited date");
    user->destroy(clock);
    dead([&] {
        (void)user->business_time();
    });
    dead([&] {
        (void)user->business_date();
    });

    FixedBusinessClock unmanaged(timestamp);
    bool unmanaged_rejected = false;
    try {
        user->set_business_clock(*user, unmanaged);
    } catch (const std::invalid_argument&) {
        unmanaged_rejected = true;
    }
    check(unmanaged_rejected, "Unmanaged clock cannot bypass explicit Context ownership");
    dead([&] {
        (void)user->business_time();
    });

    auto& override_date = user->make<BusinessDate>(*Date::from_ymd(2040, 1, 2));
    user->insert_resource(*user, override_date);
    check(user->business_date() == override_date.value,
          "Date override precedes the destroyed clock without invoking it");
    dead([&] {
        (void)user->business_time();
    });

    auto shared = user->make_global<FixedBusinessClock>(timestamp);
    auto next = global.make_user_context();
    user->set_business_clock(global, user->get(shared));
    next->set_business_clock(global, next->get(next->rebind_global(shared)));
    check(next->business_time() == timestamp, "Global clock supports independent User borrows");
    user->destroy(shared);
    dead([&] {
        (void)user->business_time();
    });
    dead([&] {
        (void)next->business_time();
    });
    dead([&] {
        (void)next->business_date();
    });

    auto replacement = next->make_ref<FixedBusinessClock>(timestamp);
    next->set_business_clock(*next, next->get(replacement));
    bool during_close = false;

    struct Closing {
        UserContext* user;
        UtcDateTime timestamp;
        bool* observed;
    };

    Closing closing{next.get(), timestamp, &during_close};
    Arena::Quiescence stop;
    stop.attach(*dynamic_cast<Arena*>(next->memory_resource()), &closing, [](void* raw) noexcept {
        auto& state = *static_cast<Closing*>(raw);
        *state.observed = state.user->business_time() == state.timestamp;
    });
    next.reset();
    check(during_close, "Checked clock remains readable during owner quiescence");
    user.reset();
    check(global.live_user_bytes() == 0, "All User clock borrow storage is reclaimed");
    std::cout << "Checked business clock lifecycle contracts pass\n";
}
