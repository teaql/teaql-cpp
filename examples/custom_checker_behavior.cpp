#include "generated/include/cpp_examples.hpp"
#include <iostream>

using namespace cpp_examples;

// Application-owned extensions stay outside regenerated domain headers.
class ProductRules final : public ProductCheckerLogic {
public:
    using ProductCheckerLogic::ProductCheckerLogic;
    mutable std::size_t calls = 0;

    void checkAndFixProduct(const UserContext&,
                            Product& product,
                            CheckObjectStatus,
                            const ObjectLocation& location,
                            CheckResults& results) const override {
        ++calls;
        requiredText(product.name(), "name", location, results);
    }
};

class ProductOrder final : public ProductBehavior {
public:
    using ProductBehavior::ProductBehavior;
    mutable std::size_t calls = 0;

    RuntimeHookResult
    before_select(Memory r, const UserContext&, SelectQuery& query) const override {
        ++calls;
        query.order_by.emplace_back(OrderBy::asc(r, "name"));
        return RuntimeHookResult::ok(r, {});
    }
};

static void run(UserContext& user_context) {
    configureRuntime(user_context);
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    auto& rules = user_context.make<ProductRules>();
    auto& checker = user_context.make<ProductChecker>(rules);
    auto& adapter =
        user_context.make<TypedEntityChecker<Product, ProductChecker>>(user_context, checker);
    auto& behavior = user_context.make<ProductOrder>();
    module.checker(user_context, adapter);
    module.entity_with_behavior<Product>(user_context, behavior);
    module.apply_to(user_context);
    require_value(user_context.ensure_schema().wait());
    auto Q = user_context.queries<query_factory>();
    for (auto name : {"visible", "hidden", ""}) {
        auto& product = Q.products()
                            .comment("Prepare an example product")
                            .purpose("Exercise application-owned validation")
                            .newEntity(user_context);
        product.updateName(name);
        const auto& result =
            AuditedSaveExt<Product>::save(
                std::move(product).audit_as("Demonstrate a custom Product Checker"), user_context)
                .wait();
        if (bool(result) != !std::string_view(name).empty()) {
            throw std::runtime_error(
                "Unexpected custom Checker result for '" + std::string(name) + "': " +
                (result ? "save succeeded"
                        : std::string(result.error().to_string(user_context.memory_resource()))));
        }
    }
    const auto& rows =
        require_value(Q.products()
                          .selectName()
                          .comment("Read products through the custom Behavior")
                          .purpose("Demonstrate application-owned query customization")
                          .executeForList(user_context)
                          .wait());
    if (rows.len() != 2 || E::product(rows[0]).getName().unwrap() != "hidden" ||
        E::product(rows[1]).getName().unwrap() != "visible" || rules.calls != 3 ||
        behavior.calls == 0) {
        throw std::runtime_error("Custom extensions did not run through the runtime");
    }
    std::cout << "custom-checker: rejected blank name; custom-behavior: hidden, visible\n";
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context_owner = global.make_user_context();
        run(*user_context_owner);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
