#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory_resource>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#if defined(TEAQL_USE_MODULES)
import teaql_runtime;
#else
#include <teaql/runtime/generated_business_id_environment.hpp>
#include <teaql/runtime/context.hpp>
#endif

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

class InitialSequence final : public BusinessIdAllocator {
public:
    using allocator_type = Allocator;

    InitialSequence(std::allocator_arg_t, Allocator) {}

    Result<BusinessIdAllocation, BusinessIdError>
    allocate(Memory resource, const BusinessIdPlan& plan) const override {
        return Result<BusinessIdAllocation, BusinessIdError>::ok(
            resource,
            BusinessIdAllocation(
                std::allocator_arg, Allocator(resource), plan.scope, plan.initial_sequence));
    }
};

struct Slot {
    std::optional<BusinessIdValue> value;

    std::optional<std::string_view> current_business_id() const {
        return value ? std::optional<std::string_view>(value->value) : std::nullopt;
    }

    bool is_new_aggregate() const {
        return true;
    }

    void assign_business_id(BusinessIdValue assigned) {
        value.emplace(std::move(assigned));
    }
};

static void contracts() {
    GlobalContext global;
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    auto resource = user_context.memory_resource();
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    {
        auto parsed = read_generated_business_id_key(resource);
        check(bool(parsed), "The controlled environment supplies a valid key");
        std::array<std::uint8_t, 32> bytes{};
        for (std::size_t index = 0; index != bytes.size(); ++index) {
            bytes[index] = static_cast<std::uint8_t>(index);
        }
        auto expected = BusinessIdEncodingKey::create(resource, 1, bytes);
        check(parsed.value() == expected.value(), "Every key byte and version is preserved");
        String signed_bytes(resource);
        for (std::size_t index = 0; index != bytes.size(); ++index) {
            signed_bytes += "+A";
        }
        check(::setenv("TEAQL_BUSINESS_ID_KEY_V1_HEX", signed_bytes.c_str(), 1) == 0,
              "The fixture supplies the source's signed-positive hexadecimal pairs");
        auto signed_key = read_generated_business_id_key(resource);
        std::array<std::uint8_t, 32> tens{};
        tens.fill(10);
        check(signed_key &&
                  signed_key.value() == BusinessIdEncodingKey::create(resource, 1, tens).value(),
              "The source radix parser accepts each +A pair as byte ten");
        String broken_boundary(resource);
        for (std::size_t index = 0; index != 20; ++index) {
            broken_boundary += "\xe2\x82\xac";
        }
        broken_boundary += "0000";
        check(::setenv("TEAQL_BUSINESS_ID_KEY_V1_HEX", broken_boundary.c_str(), 1) == 0,
              "The fixture supplies a complete Unicode key with a broken source slice boundary");
        bool interrupted = false;
        try {
            (void)read_generated_business_id_key(resource);
        } catch (const std::out_of_range&) {
            interrupted = true;
        }
        check(interrupted,
              "The source panic remains an interrupted path, not a typed Config result");
        check(::setenv("TEAQL_BUSINESS_ID_KEY_V1_HEX",
                       "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
                       1) == 0,
              "The fixture restores its controlled key");
        check(::unsetenv("TEAQL_BUSINESS_ID_KEY_VERSION") == 0,
              "The fixture can test the source default version");
        auto default_version = read_generated_business_id_key(resource);
        check(default_version && default_version.value() == expected.value(),
              "A missing version selects one");
        check(::setenv("TEAQL_BUSINESS_ID_KEY_VERSION", "\xff", 1) == 0,
              "The fixture can supply a non-Unicode version");
        auto invalid_unicode_version = read_generated_business_id_key(resource);
        check(invalid_unicode_version && invalid_unicode_version.value() == expected.value(),
              "The source also defaults a non-Unicode version to one");
        check(::setenv("TEAQL_BUSINESS_ID_KEY_VERSION", "+1", 1) == 0,
              "The fixture can supply a signed-positive version");
        auto signed_version = read_generated_business_id_key(resource);
        check(signed_version && signed_version.value() == expected.value(),
              "Positive signs retain the source unsigned parsing behavior");
        check(::setenv("TEAQL_BUSINESS_ID_KEY_VERSION", "1", 1) == 0,
              "The fixture restores its controlled version");
        auto no_service = install_generated_business_id_environment_key(user_context);
        check(!no_service, "Key installation requires actual Business-ID infrastructure");
        auto& allocator = user_context.make<InitialSequence>();
        auto& service = user_context.make<BusinessIdService>(user_context, allocator);
        user_context.set_business_id_service(user_context, service);
        check(bool(install_generated_business_id_environment_key(user_context)),
              "The parsed provider is installed as a managed UserContext object");
        auto& clock = user_context.make<FixedBusinessClock>(
            UtcDateTime::from_timestamp(1'790'985'600, 0).value());
        user_context.set_business_clock(user_context, clock);
        auto definition =
            BusinessIdDefinition::daily_sequence(resource, "business_id", "ORD", "orders");
        Slot slot;
        auto generated = user_context.business_ids().value().get().ensure(
            resource, user_context, definition, "Tenant:7", "Order", slot);
        check(generated && generated.value().value == "ORD-20261003-EWCLPK" && slot.value,
              "The installed bytes reach the existing source-qualified permutation profile");
        auto* installed = &user_context.business_ids().value().get();
        check(::setenv("TEAQL_BUSINESS_ID_KEY_VERSION", "0", 1) == 0,
              "The fixture can change its synthetic environment");
        auto invalid = install_generated_business_id_environment_key(user_context);
        check(!invalid && &user_context.business_ids().value().get() == installed,
              "Failed parsing leaves the current managed key service installed");
        check(invalid.error().memory_resource() == resource,
              "Typed configuration errors belong to the caller's allocation scope");
        check(::setenv("TEAQL_BUSINESS_ID_KEY_VERSION", "1", 1) == 0,
              "The fixture restores its controlled version");
    }
    auto service = user_context.business_ids();
    user_context_owner.reset();
    check(global.live_user_bytes() == 0, "Actual User destruction returns every allocation block");
    bool expired = false;
    try {
        (void)service.value().get();
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Closed;
    }
    check(expired, "Installed key services remain checked borrows after User destruction");
    std::pmr::set_default_resource(previous);
}

int main(int argc, char**) {
    try {
        if (argc > 1) {
            GlobalContext global;
            auto user_context_owner = global.make_user_context();
            auto& user_context = *user_context_owner;
            auto resource = user_context.memory_resource();
            auto key = read_generated_business_id_key(resource);
            Json::Object result(resource);
            result.emplace("key_loaded", Json(resource, bool(key)));
            if (key) {
                result.emplace("version", Json(resource, key.value().version()));
            } else {
                Json::Object error(resource);
                error.emplace("display", Json(resource, key.error().to_string(resource)));
                error.emplace("debug", Json(resource, debug_string(resource, key.error())));
                auto cause = key.error().source(resource);
                error.emplace("source", cause ? Json(resource, *cause) : Json(resource));
                result.emplace("error", Json(resource, std::move(error)));
            }
            std::cout << Json(resource, std::move(result)).dump(resource) << '\n';
        } else {
            contracts();
            std::cout << "Managed Business-ID environment contracts passed\n";
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
