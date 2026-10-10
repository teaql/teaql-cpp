#include <teaql/runtime/context.hpp>
#include <cstdlib>
#include <exception>
using namespace teaql;
[[noreturn]] static void close_and_report(const UserContext& context) {
    const_cast<UserContext&>(context).close();
    std::_Exit(1);
}
struct Slot {
    const UserContext& context;
    bool close_current;
    std::optional<std::string_view> current_business_id() const {
        if (close_current) { close_and_report(context); }
        return {};
    }
    bool is_new_aggregate() const { return true; }
    void assign_business_id(BusinessIdValue) {}
};
struct Key final : BusinessIdKeyProvider {
    Result<BusinessIdEncodingKey, BusinessIdError> current_key(Memory, const UserContext& context,
        const BusinessIdDefinition&) const override { close_and_report(context); }
};
struct Allocation final : BusinessIdAllocator {
    const UserContext& context;
    explicit Allocation(const UserContext& context) : context(context) {}
    Result<BusinessIdAllocation, BusinessIdError> allocate(Memory, const BusinessIdPlan&) const override {
        close_and_report(context);
    }
};
int main(int argc, char** argv) {
    if (argc != 2) { return 3; }
    std::set_terminate([] { std::_Exit(0); });
    try {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto mode = std::string_view(argv[1]);
        auto& allocator = global.make<InMemoryBusinessIdAllocator>();
        auto& closing = global.make_resource<Allocation>(*user);
        auto& service = mode == "allocator" ? global.make<BusinessIdService>(global, closing)
                                           : global.make<BusinessIdService>(global, allocator);
        auto definition = BusinessIdDefinition::legacy_daily_sequence(r, "number", "ORD", "order");
        if (mode == "key") {
            definition.profile = DEFAULT_BUSINESS_ID_PROFILE;
            auto& key = global.make_resource<Key>();
            service.with_key_provider(global, key);
        }
        Slot slot{*user, mode == "slot"};
        (void)service.ensure(r, *user, definition, "Root", "Order", slot);
    } catch (...) { std::_Exit(2); }
    return 4;
}
