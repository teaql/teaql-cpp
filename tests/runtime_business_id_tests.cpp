#include <teaql/runtime/business_id.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}

static void profiles_and_domains() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto g = global.memory_resource();
    std::array<std::uint8_t, 32> bytes{};
    for (unsigned i = 0; i < 32; ++i)
        bytes[i] = std::uint8_t(i);
    auto& key =
        user->make<BusinessIdEncodingKey>(BusinessIdEncodingKey::create(r, 1, bytes).value());
    auto& provider = user->make<StaticBusinessIdKeyProvider>(key);
    auto& retained_provider = global.make<StaticBusinessIdKeyProvider>(provider);
    auto& v1 = user->make<PermutedDailyBusinessIdProfile>(key);
    auto& retained_v1 = global.make<PermutedDailyBusinessIdProfile>(v1);
    auto& legacy = global.make<DailySequenceBusinessIdProfile>();
    auto& scope = user->make<BusinessIdScope>("Tenant:7", "Order", "orders", "20261003");
    auto& encoded = global.make<Result<String, BusinessIdError>>(
        BusinessIdPermutationV1::encode(r, 0, scope, key));
    auto& invalid = global.make<Result<BusinessIdEncodingKey, BusinessIdError>>(
        BusinessIdEncodingKey::create(r, 0, bytes));
    auto& definition = user->make<BusinessIdDefinition>("business_id", "ORD", "orders");
    auto& request = user->make<BusinessIdGenerationRequest>(
        definition, "Tenant:7", "Order", *Date::from_ymd(2026, 10, 3));
    auto& plan = user->make<BusinessIdPlan>(v1.plan(r, request).value());
    auto& retained_plan = global.make<BusinessIdPlan>(plan);
    auto& allocation = user->make<BusinessIdAllocation>(scope, 0);
    auto& value =
        global.make<Result<BusinessIdValue, BusinessIdError>>(v1.format(r, plan, allocation));
    check(value.value().value == "ORD-20261003-EWCLPK" && plan.initial_sequence == 0 &&
              plan.max_sequence == BUSINESS_ID_V1_DOMAIN_SIZE - 1,
          "permuted profile keeps its exact allocation range and Rust encoding");
    definition.profile = LEGACY_DAILY_SEQUENCE_PROFILE;
    definition.preserve_digits = 8;
    auto& legacy_request = user->make<BusinessIdGenerationRequest>(
        definition, "Tenant:7", "Order", *Date::from_ymd(2026, 10, 3));
    auto legacy_plan = legacy.plan(g, legacy_request);
    check(legacy_plan.value().initial_sequence == 1 && legacy_plan.value().max_sequence == 99999999,
          "legacy sequence is a distinct profile with a different initial value");
    check(legacy.format(g, legacy_plan.value(), allocation).value().value ==
              "ORD-20261003-00000000",
          "format does not invent an initial-sequence lower bound absent in Rust");
    user.reset();
    check(encoded.value() == "EWCLPK" && encoded.value().get_allocator().resource() == g &&
              value.value().memory_resource() == g,
          "explicit result copies survive source Context close");
    check(invalid.error().code == BusinessIdErrorCode::InvalidDefinition &&
              invalid.error().memory_resource() == g,
          "key creation errors copy into the destination domain");
    auto next = global.make_user_context();
    auto n = next->memory_resource();
    auto& d = next->make<BusinessIdDefinition>("id", "ORD", "orders");
    auto supplied = retained_provider.current_key(n, *next, d);
    check(supplied.value().version() == 1,
          "retained key provider owns inline key bytes rather than a User borrow");
    auto& a = next->make<BusinessIdAllocation>(retained_plan.scope, 1);
    check(retained_v1.format(n, retained_plan, a).value().value == "ORD-20261003-B7GGH3",
          "retained profile may allocate results into a fresh UserContext");
    // Validation in the source checks syntax; it is not key verification or plan validation.
    d.namespace_name = "";
    d.preserve_digits = 0;
    d.split_by_date = false;
    check(
        retained_v1.validate(n, d, "ORD-20261003-ZZZZZZ").is_ok(),
        "V1 validation does not add namespace, width configuration, or split checks absent in Rust");
    d.profile = LEGACY_DAILY_SEQUENCE_PROFILE;
    d.namespace_name = "orders";
    d.preserve_digits = 0;
    d.split_by_date = false;
    check(legacy.validate(n, d, "ORD-").is_ok(),
          "legacy validate accepts an empty sequence when configured with zero width");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        profiles_and_domains();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime Business ID profile contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
