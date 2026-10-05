#include "../internal_id_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <chrono>
#include <iostream>

static Json error(Memory r, const RuntimeError& e) {
    auto out = detail::model_json(r);
    detail::model_put(out, "message", e.message());
    detail::model_put(out, "display", e.to_string(r));
    return out;
}

static Json ok(Memory r, Json value) {
    auto out = detail::model_json(r);
    detail::model_put(out, "ok", value);
    return out;
}

static Json numeric(Memory r, std::uint64_t value) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), value).ptr;
    return Json(r, std::string_view(buffer, end));
}

struct Fixed : InternalIdGenerator {
    bool fail;

    explicit Fixed(bool fail) : fail(fail) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        using R = Result<std::uint64_t, RuntimeError>;
        return fail ? R::err(r, RuntimeError::id_generation(r, "unavailable")) : R::ok(r, 0);
    }
};

static Json evaluate(UserContext& context, const Json& j) {
    auto r = context.memory_resource();
    auto kind = text(field(j, "kind"));
    if (kind == "snowflake") {
        try {
            auto worker = number<std::uint64_t>(field(j, "worker")),
                 dc = number<std::uint64_t>(field(j, "datacenter"));
            auto& generator = context.make<SnowflakeIdGenerator>(worker, dc);
            generator.ensure_floor(r, "ignored", UINT64_MAX);
            auto millis = []() {
                return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::system_clock::now().time_since_epoch())
                                         .count());
            };
            auto before = millis();
            std::optional<std::uint64_t> previous;
            bool valid = true;
            for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(j, "count").data); ++i) {
                auto id = generator.generate_id(r, "Order").value();
                valid = valid && ((id >> 12) & 31) == worker && ((id >> 17) & 31) == dc &&
                        (!previous || id > *previous) && (id >> 22) + 1288834974657 >= before;
                previous = id;
            }
            auto after = millis();
            valid = valid && (!previous || (*previous >> 22) + 1288834974657 <= after);
            auto out = detail::model_json(r);
            detail::model_put(out, "valid", valid);
            return out;
        } catch (const std::invalid_argument& e) {
            auto out = detail::model_json(r);
            detail::model_put(out, "panic", std::string_view(e.what()));
            return out;
        }
    }
    InternalIdGenerator* generator = nullptr;
    if (kind == "atomic") {
        generator =
            &context.make<AtomicCounterIdGenerator>(number<std::uint64_t>(field(j, "start")));
    } else if (kind == "fixed" || kind == "error") {
        generator = &context.make_resource<Fixed>(kind == "error");
    }
    if (generator) {
        context.set_internal_id_generator(context, *generator);
    }
    auto& model = context.make<Model>();
    Json::Array out(r);
    for (const auto& step : std::get<Json::Array>(field(j, "steps").data)) {
        auto op = text(field(step, "op")), entity = text(field(step, "entity"));
        if (op == "canonical") {
            out.emplace_back(canonical_id_space_entity(r, entity));
            continue;
        }
        if (op == "generate") {
            auto result = context.generate_id(entity);
            out.emplace_back(result ? ok(r, result.value() ? numeric(r, *result.value()) : Json(r))
                                    : error(r, result.error()));
            continue;
        }
        if (op == "next") {
            auto result = context.next_id(entity);
            out.emplace_back(result ? ok(r, numeric(r, result.value())) : error(r, result.error()));
            continue;
        }
        if (op == "floor") {
            auto result =
                generator->ensure_floor(r, entity, number<std::uint64_t>(field(step, "floor")));
            out.emplace_back(result ? ok(r, Json(r)) : error(r, result.error()));
            continue;
        }
        if (op == "bootstrap") {
            model.fresh = std::get<bool>(field(step, "fresh").data);
            auto result = context.initialize_generated_bootstrap_entity(
                model, entity, number<std::uint64_t>(field(step, "floor")));
            auto item = detail::model_json(r);
            detail::model_put(item, "result", result ? ok(r, Json(r)) : error(r, result.error()));
            detail::model_put(item, "fresh", model.fresh);
            detail::model_put(item, "id", numeric(r, model.id));
            out.emplace_back(std::move(item));
        }
    }
    return Json(r, out);
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto context = global.make_user_context();
        auto r = context->memory_resource();
        auto j = Json::parse(r, line);
        if (!j) {
            throw std::runtime_error("bad fixture JSON");
        }
        std::cout << evaluate(*context, *j).dump(r) << '\n';
    }
}
