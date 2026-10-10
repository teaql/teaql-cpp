#include <teaql/runtime.hpp>
#include <teaql/runtime/runtime_id_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <iostream>

static Json numeric(Memory owner, std::uint64_t value) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), value).ptr;
    return Json(owner, std::string_view(buffer, end));
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto output = detail::model_json(owner);
    auto put = [&](std::string_view name, auto&& value) {
        detail::model_put(output, name, std::forward<decltype(value)>(value));
    };
    auto kind = text(field(input, "kind"));
    if (kind == "clock") {
        auto time = UtcDateTime::from_timestamp(
            number<std::int64_t>(field(input, "seconds")),
            detail::model_integer<std::uint32_t>(field(input, "nanos"), "nanos"));
        if (!time) {
            throw std::invalid_argument("Invalid clock fixture");
        }
        SystemBusinessClock system;
        FixedBusinessClock fixed(*time);
        BusinessDate date{time->date_naive()};
        DailySequenceBusinessIdProfile profile;
        put("system", formatted_debug(owner, system));
        put("system_clone", formatted_debug(owner, SystemBusinessClock(system)));
        put("fixed", formatted_debug(owner, fixed));
        put("fixed_clone", formatted_debug(owner, FixedBusinessClock(fixed)));
        put("date", formatted_debug(owner, date));
        put("date_clone", formatted_debug(owner, BusinessDate(date)));
        put("date_equal", date == BusinessDate(date));
        put("profile", formatted_debug(owner, profile));
        put("profile_clone", formatted_debug(owner, DailySequenceBusinessIdProfile(profile)));
        put("time", formatted_debug(owner, fixed.business_time()));
        put("business_date", formatted_debug(owner, fixed.business_date()));
    } else if (kind == "key") {
        std::array<std::uint8_t, 32> bytes{}, other_bytes{};
        bytes.fill(detail::model_integer<std::uint8_t>(field(input, "byte"), "byte"));
        other_bytes.fill(
            detail::model_integer<std::uint8_t>(field(input, "other_byte"), "other_byte"));
        auto key = BusinessIdEncodingKey::create(
            owner, detail::model_integer<std::uint32_t>(field(input, "version"), "version"), bytes);
        if (!key) {
            put("error", key.error().to_string(owner));
        } else {
            auto other =
                BusinessIdEncodingKey::create(owner,
                                              detail::model_integer<std::uint32_t>(
                                                  field(input, "other_version"), "other_version"),
                                              other_bytes);
            put("version", key.value().version());
            put("clone_equal", key.value() == BusinessIdEncodingKey(key.value()));
            put("other_equal", key.value() == other.value());
        }
    } else if (kind == "snowflake") {
        try {
            auto& generator =
                user.make<SnowflakeIdGenerator>(number<std::uint64_t>(field(input, "worker")),
                                                number<std::uint64_t>(field(input, "datacenter")));
            put("debug", formatted_debug(owner, generator));
        } catch (const std::invalid_argument& error) {
            put("panic", std::string_view(error.what()));
        }
    } else {
        Json::Array observations(owner);
        if (kind == "counter") {
            auto start = std::get<bool>(field(input, "default").data)
                             ? 1000
                             : number<std::uint64_t>(field(input, "start"));
            auto& generator = user.make<AtomicCounterIdGenerator>(start);
            auto initial = detail::model_json(owner);
            detail::model_put(initial, "debug", formatted_debug(owner, generator));
            observations.emplace_back(std::move(initial));
            for (const auto& action : detail::model_array(field(input, "steps"), "steps")) {
                auto result = detail::model_json(owner);
                try {
                    if (text(field(action, "op")) == "floor") {
                        auto reserved = generator.ensure_floor(
                            owner, "Ignored", number<std::uint64_t>(field(action, "value")));
                        if (reserved) {
                            detail::model_put(result, "ok", Json(owner));
                        } else {
                            detail::model_put(result, "error", reserved.error().to_string(owner));
                        }
                    } else {
                        auto generated = generator.generate_id(owner, "Ignored");
                        if (generated) {
                            detail::model_put(result, "ok", numeric(owner, generated.value()));
                        } else {
                            detail::model_put(result, "error", generated.error().to_string(owner));
                        }
                    }
                } catch (const std::overflow_error& error) {
                    detail::model_put(result, "panic", std::string_view(error.what()));
                }
                auto step = detail::model_json(owner);
                detail::model_put(step, "result", result);
                detail::model_put(step, "debug", formatted_debug(owner, generator));
                observations.emplace_back(std::move(step));
            }
        } else if (kind == "allocator") {
            auto& allocator = user.make<InMemoryBusinessIdAllocator>();
            auto initial = detail::model_json(owner);
            detail::model_put(initial, "debug", formatted_debug(owner, allocator));
            observations.emplace_back(std::move(initial));
            for (const auto& action : detail::model_array(field(input, "steps"), "steps")) {
                BusinessIdScope scope(
                    std::allocator_arg, Allocator(owner), "Root|%", "Item", "sequence", "20261008");
                BusinessIdPlan plan(std::allocator_arg,
                                    Allocator(owner),
                                    scope,
                                    "ORD",
                                    {},
                                    6,
                                    "-",
                                    number<std::uint64_t>(field(action, "initial")),
                                    number<std::uint64_t>(field(action, "maximum")));
                auto allocation = allocator.allocate(owner, plan);
                auto result = detail::model_json(owner);
                if (allocation) {
                    detail::model_put(
                        result, "sequence", numeric(owner, allocation.value().sequence));
                    detail::model_put(
                        result, "scope", allocation.value().scope.canonical_key(owner));
                } else {
                    detail::model_put(result, "error", allocation.error().to_string(owner));
                }
                auto step = detail::model_json(owner);
                detail::model_put(step, "result", result);
                detail::model_put(step, "debug", formatted_debug(owner, allocator));
                observations.emplace_back(std::move(step));
            }
        } else {
            throw std::invalid_argument("Unknown runtime ID fixture");
        }
        return Json(owner, std::move(observations));
    }
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid runtime ID fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
