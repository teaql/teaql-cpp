#include "../generated_types/include/cpp_types.hpp"
#include "../generated_optional_types/include/cpp_optional_types.hpp"
#include "fixture_value.hpp"
#include <iostream>

// Every owning input/result leaves its scope before the actual User deletion.
template <class F>
static Json capture(Memory r, F&& function) {
    auto output = detail::model_json(r);
    try {
        detail::model_put(output, "ok", function());
    } catch (const ExpressionNotLoadedError& error) {
        detail::model_put(output, "panic", error.diagnostic().to_json(r));
    } catch (const std::logic_error& error) {
        detail::model_put(output, "panic", std::string_view(error.what()));
    }
    return output;
}

template <class T, class F>
static Json
summary(Memory r, const RuntimeExpression<T>& expression, const T& fallback, F&& encode) {
    int callbacks = 0;
    auto evaluation = capture(r, [&] {
        auto value = expression.eval();
        auto output = detail::model_json(r);
        detail::model_put(output, "present", bool(value));
        detail::model_put(output, "value", value ? encode(*value) : Json(r));
        return output;
    });
    auto unwrapped = capture(r, [&] {
        return encode(expression.unwrap());
    });
    auto eager = capture(r, [&] {
        return encode(expression.orIfNull(detail::clone_in(r, fallback)));
    });
    auto lazy = capture(r, [&] {
        return encode(expression.orElseIfNull([&] {
            ++callbacks;
            return detail::clone_in(r, fallback);
        }));
    });
    auto default_value = capture(r, [&] {
        return encode(expression.orDefaultIfNull());
    });
    auto output = detail::model_json(r);
    detail::model_put(output, "eval", std::move(evaluation));
    detail::model_put(output, "unwrap", std::move(unwrapped));
    detail::model_put(output, "eager", std::move(eager));
    detail::model_put(output, "lazy", std::move(lazy));
    detail::model_put(output, "default", std::move(default_value));
    detail::model_put(output, "callbacks", callbacks);
    return output;
}

static CompactRow row(Memory r, const Json& input, bool fallback) {
    auto name = text(field(input, "field"));
    Record fields(r);
    fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    fields.emplace(String("version", r), Value(r, std::int64_t(1)));
    fields.erase(String(name, r));
    if (fallback || std::get<bool>(field(input, "loaded").data)) {
        fields.emplace(String(name, r), value(r, field(input, fallback ? "fallback" : "value")));
    }
    return CompactRow(std::allocator_arg, Allocator(r), fields);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto name = text(field(input, "field"));
    auto model = text(field(input, "model"));
    if (model == "Person") {
        auto decoded = cpp_types::Person::from_compact_row(r, row(r, input, false));
        if (!decoded) {
            auto output = detail::model_json(r);
            detail::model_put(output, "decode_error", decoded.error().to_string(r));
            return output;
        }
        auto entity = std::move(decoded).value();
        auto fallback = cpp_types::Person::from_compact_row(r, row(r, input, true)).value();
        auto expression = cpp_types::E::person(entity);
        if (name == "id") {
            return summary(r, expression.getId(), fallback.id(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "email") {
            return summary(r, expression.getEmail(), fallback.email(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "active") {
            return summary(r, expression.getActive(), fallback.active(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "nickname") {
            return summary(r, expression.getNickname(), fallback.nickname(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "balance") {
            return summary(r, expression.getBalance(), fallback.balance(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "height") {
            return summary(r, expression.getHeight(), fallback.height(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "ratio") {
            return summary(r, expression.getRatio(), fallback.ratio(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "age") {
            return summary(r, expression.getAge(), fallback.age(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "birthday") {
            return summary(r, expression.getBirthday(), fallback.birthday(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "updated_at") {
            return summary(r, expression.getUpdatedAt(), fallback.updatedAt(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "payload") {
            return summary(r, expression.getPayload(), fallback.payload(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "version") {
            return summary(r, expression.getVersion(), fallback.version(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
    }
    if (model == "Reading") {
        auto decoded = cpp_optional_types::Reading::from_compact_row(r, row(r, input, false));
        if (!decoded) {
            auto output = detail::model_json(r);
            detail::model_put(output, "decode_error", decoded.error().to_string(r));
            return output;
        }
        auto entity = std::move(decoded).value();
        auto fallback =
            cpp_optional_types::Reading::from_compact_row(r, row(r, input, true)).value();
        auto expression = cpp_optional_types::E::reading(entity);
        if (name == "id") {
            return summary(r, expression.getId(), fallback.id(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "text") {
            return summary(r, expression.getText(), fallback.text(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "active") {
            return summary(r, expression.getActive(), fallback.active(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "balance") {
            return summary(r, expression.getBalance(), fallback.balance(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "height") {
            return summary(r, expression.getHeight(), fallback.height(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "ratio") {
            return summary(r, expression.getRatio(), fallback.ratio(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "count") {
            return summary(r, expression.getCount(), fallback.count(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "day") {
            return summary(r, expression.getDay(), fallback.day(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "recorded_at") {
            return summary(
                r, expression.getRecordedAt(), fallback.recordedAt(), [&](const auto& v) {
                    return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
                });
        }
        if (name == "payload") {
            return summary(r, expression.getPayload(), fallback.payload(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
        if (name == "version") {
            return summary(r, expression.getVersion(), fallback.version(), [&](const auto& v) {
                return cpp_optional_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
    }
    if (model == "Device") {
        auto decoded = cpp_types::Device::from_compact_row(r, row(r, input, false));
        if (!decoded) {
            auto output = detail::model_json(r);
            detail::model_put(output, "decode_error", decoded.error().to_string(r));
            return output;
        }
        auto entity = std::move(decoded).value();
        auto fallback = cpp_types::Device::from_compact_row(r, row(r, input, true)).value();
        auto expression = cpp_types::E::device(entity);
        if (name == "owner_id") {
            return summary(r, expression.getOwnerId(), fallback.ownerId(), [&](const auto& v) {
                return cpp_types::generated_detail::encode(r, v).to_json_value(r);
            });
        }
    }
    throw std::logic_error("Unknown expression model/field");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto r = user_context->memory_resource();
                auto input = Json::parse(r, line);
                std::cout << observe(*user_context, *input).dump(r) << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Expression observer retained User arena storage");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
