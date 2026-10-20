#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include "fixture_value.hpp"
#include <iostream>
using namespace cpp_business_ids;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

template <class F>
static Json capture(Memory r, F&& function) {
    auto output = detail::model_json(r);
    try {
        detail::model_put(output, "ok", function());
    } catch (const ExpressionNotLoadedError& error) {
        detail::model_put(output, "panic", error.diagnostic().to_json(r));
    }
    return output;
}

template <class T>
static Json summary(Memory r, const RuntimeExpression<T>& expression) {
    int callbacks = 0;
    auto evaluation = capture(r, [&] {
        auto v = expression.eval();
        auto output = detail::model_json(r);
        detail::model_put(output, "present", bool(v));
        detail::model_put(output, "value", v ? v->toValue(r).to_json_value(r) : Json(r));
        return output;
    });
    auto unwrapped = capture(r, [&] {
        return expression.unwrap().toValue(r).to_json_value(r);
    });
    auto eager = capture(r, [&] {
        return expression.orIfNull(std::move(T::create(r, "fallback")).value())
            .toValue(r)
            .to_json_value(r);
    });
    auto lazy = capture(r, [&] {
        return expression
            .orElseIfNull([&] {
                ++callbacks;
                return std::move(T::create(r, "fallback")).value();
            })
            .toValue(r)
            .to_json_value(r);
    });
    auto output = detail::model_json(r);
    detail::model_put(output, "eval", evaluation);
    detail::model_put(output, "unwrap", unwrapped);
    detail::model_put(output, "eager", eager);
    detail::model_put(output, "lazy", lazy);
    detail::model_put(output, "callbacks", callbacks);
    return output;
}

static CompactRow row(Memory r, const Json& input, std::string_view name) {
    CompactRow fields(std::allocator_arg, Allocator(r));
    fields.insert("id", Value(r, std::get<std::uint64_t>(field(input, "id").data)));
    fields.insert("version", Value(r, std::int64_t(1)));
    if (std::get<bool>(field(input, "loaded").data)) {
        fields.insert(name, value(r, field(input, "value")));
    }
    return fields;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto model = text(field(input, "model"));
    if (model == "Receipt") {
        auto decoded = Receipt::from_compact_row(r, row(r, input, "receipt_number"));
        if (!decoded) {
            auto output = detail::model_json(r);
            detail::model_put(output, "decode_error", decoded.error().to_string(r));
            return output;
        }
        auto entity = std::move(decoded).value();
        auto expression = E::receipt(entity).getReceiptNumber();
        auto& alias = user_context.make<Receipt>(entity);
        auto before = summary(r, expression);
        alias.updateReceiptNumber(std::move(ReceiptNumber::create(r, "replacement")).value());
        auto output = detail::model_json(r);
        detail::model_put(output, "before", before);
        detail::model_put(output, "captured", summary(r, expression));
        detail::model_put(output, "current", summary(r, E::receipt(entity).getReceiptNumber()));
        return output;
    }
    if (model == "Ticket") {
        auto decoded = Ticket::from_compact_row(r, row(r, input, "ticket_number"));
        if (!decoded) {
            auto output = detail::model_json(r);
            detail::model_put(output, "decode_error", decoded.error().to_string(r));
            return output;
        }
        auto entity = std::move(decoded).value();
        auto expression = E::ticket(entity).getTicketNumber();
        auto& alias = user_context.make<Ticket>(entity);
        auto before = summary(r, expression);
        alias.updateTicketNumber(std::move(TicketNumber::create(r, "replacement")).value());
        auto output = detail::model_json(r);
        detail::model_put(output, "before", before);
        detail::model_put(output, "captured", summary(r, expression));
        detail::model_put(output, "current", summary(r, E::ticket(entity).getTicketNumber()));
        return output;
    }
    throw std::logic_error("Unknown Business ID model");
}

static void lifetime() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto destination = global.make_user_context();
    auto r = source->memory_resource();
    auto& receipt = source->make<Receipt>();
    receipt.updateId(7);
    receipt.updateReceiptNumber(std::move(ReceiptNumber::create(r, String(1200, 'x', r))).value());
    auto captured = E::receipt(receipt).getReceiptNumber();
    auto retained = captured.copyTo(*destination);
    source.reset();
    bool expired = false;
    try {
        (void)captured.eval();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "Business ID E rejects deleted UserContext");
    auto value = retained.unwrap();
    check(value.asStr() == String(1200, 'x', destination->memory_resource()),
          "Explicit typed E copy retains its own text");
    check(value.memory_resource() == destination->memory_resource(),
          "Typed ID E copy binds destination allocator");
}

int main(int argc, char**) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc > 1) {
            lifetime();
        } else {
            for (std::string line; std::getline(std::cin, line);) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                {
                    auto input = Json::parse(user_context->memory_resource(), line);
                    std::cout
                        << observe(*user_context, *input).dump(user_context->memory_resource())
                        << '\n';
                }
                user_context.reset();
                check(global.live_user_bytes() == 0, "Business ID E releases User arena blocks");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
