#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejected(F&& f, const char* message) {
    bool failed = false;
    try {
        f();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

static void resources_and_domains() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto other = global.make_user_context();
    auto& shared = global.make<String>("global resource");
    auto& local = user->make<String>("user resource");
    check(!user->get_resource<String>() && user->require_resource<String>().error().kind() ==
                                               ContextError::Kind::MissingTypedResource,
          "missing typed resource retains typed error");
    user->insert_resource(global, shared);
    check(user->get_resource<String>() == &shared,
          "typed registration borrows explicitly global allocation");
    auto& global_borrow =
        global.make<ResourceBorrow<String>>(user->require_resource<String>().value());
    check(&global_borrow.get() == &shared,
          "global resource handle can be explicitly copied into Global");
    auto& sibling_borrow =
        other->make<ResourceBorrow<String>>(user->require_resource<String>().value());
    check(&sibling_borrow.get() == &shared, "global resource may be lent to another child Context");
    user->insert_resource(*user, local);
    check(user->get_resource<String>() == &local && shared == "global resource",
          "replacement changes lookup without destroying arena-owned predecessor");
    rejected(
        [&] {
            global.make<ResourceBorrow<String>>(user->require_resource<String>().value());
        },
        "required resource result cannot extend a user object's lifetime");
    rejected(
        [&] {
            other->insert_resource(*user, local);
        },
        "sibling Context cannot retain a user-owned resource");
    rejected(
        [&] {
            user->insert_resource(global, local);
        },
        "allocator-aware registration checks declared allocation origin");
    auto& number = user->make<int>(42);
    user->insert_named_resource("service", global, shared);
    check(user->get_named_resource<String>("service") == &shared,
          "named index is independent of typed index");
    user->insert_named_resource("service", *user, number);
    check(!user->get_named_resource<String>("service") &&
              *user->get_named_resource<int>("service") == 42,
          "named replacement may change resource type");
    check(user->require_named_resource<String>("service").error().to_string(r) ==
              "missing named resource: service",
          "named type mismatch uses source missing-resource error");
    static_assert(
        std::is_same_v<decltype(std::as_const(*user).get_resource<String>()), const String*>);
    check(&std::as_const(*user).require_resource<String>().value().get() == &local,
          "const Context exposes const resource borrow");
    GlobalContext unrelated;
    auto& foreign = unrelated.make<int>(3);
    rejected(
        [&] {
            user->insert_resource(unrelated, foreign);
        },
        "unrelated GlobalContext is not an implicit shared lifetime");
    user.reset();
    check(global_borrow.get() == "global resource" && sibling_borrow.get() == "global resource",
          "explicit Global borrows survive lookup Context destruction");
}

static void locals_and_cleanup() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& source = global.make<Value>(Value::Object(global.memory_resource()));
    std::get<Value::Object>(source.data)
        .emplace(String("field", global.memory_resource()),
                 Value(global.memory_resource(), "original"));
    user->put_local("data", source);
    source = Value(global.memory_resource(), "changed");
    auto* stored = user->local("data");
    check(stored && stored->memory_resource() == r &&
              std::get<Value::Object>(stored->data).begin()->second.try_text() == "original",
          "locals deep-copy nested values into UserContext");
    {
        auto removed = user->remove_local("data");
        check(removed && !user->local("data") &&
                  std::get<Value::Object>(removed->data).begin()->second.memory_resource() == r,
              "removed local retains explicit domain");
    }
    check(!user->remove_local("missing"), "missing local removal is empty");
    user->put_local("same", 1);
    user->put_local("same", "replacement");
    check(user->local("same")->try_text() == "replacement", "local replacement changes value type");

    struct Probe {
        UserContext& context;
        unsigned& destroyed;
        bool& before;

        ~Probe() {
            ++destroyed;
            before = context.allocation_stats().live_blocks > 0;
        }
    };

    unsigned destroyed = 0;
    bool before = false;
    auto& first = user->make_resource<Probe>(*user, destroyed, before);
    user->insert_resource(*user, first);
    auto& second = user->make_resource<Probe>(*user, destroyed, before);
    user->insert_resource(*user, second);
    check(destroyed == 0, "registration replacement leaves resource cleanup with allocation owner");
    bool observed = false, removal_rejected = false;
    Arena::Quiescence quiescence;

    struct Observation {
        UserContext& context;
        bool& observed;
        bool& removal_rejected;
    };

    Observation evidence{*user, observed, removal_rejected};
    quiescence.attach(*dynamic_cast<Arena*>(r), &evidence, [](void* value) noexcept {
        auto& evidence = *static_cast<Observation*>(value);
        evidence.observed = evidence.context.get_resource<Probe>() != nullptr;
        try {
            evidence.context.remove_local("same");
        } catch (const std::logic_error&) {
            evidence.removal_rejected = true;
        }
    });
    user->close();
    check(observed && removal_rejected && destroyed == 2 && before &&
              user->allocation_stats().live_blocks == 0,
          "resources remain readable during quiescence and destruct before arena release");
    rejected(
        [&] {
            user->get_resource<Probe>();
        },
        "lookup after Context close is rejected before touching reclaimed registry");
    rejected(
        [&] {
            user->local("same");
        },
        "closed locals cannot be read");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        resources_and_domains();
        locals_and_cleanup();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime Context resource contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
