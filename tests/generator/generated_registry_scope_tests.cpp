#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include <iostream>

using namespace teaql;

class CustomBehavior final : public EntityDataServiceBehavior {
public:
    using allocator_type = Allocator;

    CustomBehavior(std::allocator_arg_t, Allocator, unsigned& calls) : calls_(calls) {}

    RuntimeHookResult
    before_select(Memory resource, const UserContext&, SelectQuery& query) const override {
        ++calls_;
        query.limit(7);
        return RuntimeHookResult::ok(resource, {});
    }

private:
    unsigned& calls_;
};

class CustomChecker final : public Checker {
public:
    using allocator_type = Allocator;

    CustomChecker(std::allocator_arg_t, Allocator, unsigned& calls) : calls_(calls) {}

    std::string_view entity() const override {
        return "Tenant";
    }

    void check_and_fix(const UserContext& user_context,
                       EntityValues&,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        ++calls_;
        auto resource = user_context.memory_resource();
        auto copied_location = detail::clone_in(resource, location);
        copied_location.member("custom");
        results.push_back(CheckResult::required(resource, std::move(copied_location)));
    }

private:
    unsigned& calls_;
};

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto resource = user_context->memory_resource();
        auto& repository = cpp_business_ids::repositoryRegistry(*user_context);
        auto retained_repository = user_context->make_global<InMemoryEntityRegistry>(repository);
        auto& behavior_registry = cpp_business_ids::behaviorRegistry(*user_context);
        auto& checker_registry = cpp_business_ids::checkerRegistry(*user_context);
        auto behavior = behavior_registry.behavior(resource, "Tenant");
        auto checker = checker_registry.checker(resource, "Tenant");
        unsigned behavior_calls = 0, checker_calls = 0;
        {
            auto& changed_behaviors =
                user_context->make<InMemoryEntityDataServiceBehaviorRegistry>(behavior_registry);
            auto& custom_behavior = user_context->make<CustomBehavior>(behavior_calls);
            changed_behaviors.register_behavior("Tenant", *user_context, custom_behavior);
            SelectQuery default_query(std::allocator_arg, Allocator(resource), "Tenant");
            auto default_result =
                behavior->get().before_select(resource, *user_context, default_query);
            auto changed = changed_behaviors.behavior(resource, "Tenant");
            auto changed_result =
                changed->get().before_select(resource, *user_context, default_query);
            if (!default_result || !changed_result || behavior_calls != 1 ||
                !changed_behaviors.behavior(resource, "Receipt")) {
                throw std::runtime_error(
                    "Behavior clone replacement changed original defaults or other entities");
            }
            auto& changed_checkers = user_context->make<InMemoryCheckerRegistry>(checker_registry);
            auto& custom_checker = user_context->make<CustomChecker>(checker_calls);
            changed_checkers.register_checker(*user_context, custom_checker);
            EntityValues values(std::allocator_arg, Allocator(resource));
            CheckResults results(resource);
            auto changed_checker = changed_checkers.checker(resource, "Tenant");
            changed_checker->get().check_and_fix(
                *user_context, values, ObjectLocation::root(resource), results);
            if (checker_calls != 1 || results.size() != 1 ||
                !changed_checkers.checker(resource, "Receipt") ||
                &checker->get() == &changed_checker->get()) {
                throw std::runtime_error("Checker clone replacement lost independent registration");
            }
            bool rejected = false;
            try {
                user_context->make_global<InMemoryCheckerRegistry>(checker_registry);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            if (!rejected) {
                throw std::runtime_error(
                    "Global checker registry implicitly retained User callbacks");
            }
            rejected = false;
            try {
                user_context->make_global<InMemoryEntityDataServiceBehaviorRegistry>(
                    behavior_registry);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            if (!rejected) {
                throw std::runtime_error(
                    "Global behavior registry implicitly retained User callbacks");
            }
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Standalone registries retained User memory");
        }
        bool behavior_expired = false, checker_expired = false;
        try {
            (void)behavior->get();
        } catch (const ReferenceError&) {
            behavior_expired = true;
        }
        try {
            (void)checker->get();
        } catch (const ReferenceError&) {
            checker_expired = true;
        }
        if (!behavior_expired || !checker_expired) {
            throw std::runtime_error(
                "Checked generated registry callbacks survived their User owner");
        }
        auto next_user_context = global.make_user_context();
        auto& retained =
            next_user_context->get(next_user_context->rebind_global(retained_repository));
        if (!retained.contains("Tenant") || !retained.contains("Receipt") ||
            retained.contains("tenant")) {
            throw std::runtime_error(
                "Explicit Global repository copy changed canonical membership");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Standalone registry customization and checked callback lifetimes passed\n";
    return 0;
}
