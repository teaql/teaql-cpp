#include <cpp_privacy_contracts.hpp>
#include "fixture_value.hpp"
#include <teaql/core/entity_debug.hpp>
#include <iostream>

using namespace teaql;
namespace domain = cpp_privacy_contracts;

class Ids final : public InternalIdGenerator {
public:
    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        return Result<std::uint64_t, RuntimeError>::ok(r, 1001);
    }
};

template <class T>
static void render(DebugWriter& writer, const T& value) {
    if constexpr (requires {
                      value.has_value();
                      *value;
                  }) {
        if (value) {
            writer.tuple("Some", [&](auto& out) {
                render(out, *value);
            });
        } else {
            writer.raw("None");
        }
    } else {
        write_debug(writer, value);
    }
}

template <class T>
static Json read(const RuntimeExpression<T>& expression, std::string_view action) {
    auto r = expression.memory_resource();
    auto output = detail::model_json(r);
    try {
        String text(r);
        DebugWriter writer(text, {});
        if (action == "eval") {
            render(writer, expression.eval());
        } else {
            render(writer, expression.unwrap());
        }
        detail::model_put(output, "kind", "value");
        detail::model_put(output, "value", text);
    } catch (const ExpressionNotLoadedError& error) {
        detail::model_put(output, "kind", "not_loaded");
        detail::model_put(output, "diagnostic", error.diagnostic().to_json(r));
    } catch (const std::logic_error& error) {
        if (std::string_view(error.what()) != "Expression value is legitimately null") {
            throw;
        }
        detail::model_put(output, "kind", "null");
        detail::model_put(output, "message", error.what());
    }
    return output;
}

template <Entity T, class Request, class Factory>
static Json
inspect(UserContext& user_context, Request request, Factory factory, const Json& input) {
    auto r = user_context.memory_resource();
    auto state = text(field(input, "state"));
    auto path = text(field(input, "path"));
    auto action = text(field(input, "action"));
    auto output = detail::model_json(r);
    T* entity = nullptr;
    if (state.starts_with("json_")) {
        std::string_view payload = "{}";
        if (state == "json_null") {
            payload = R"json({"id":101,"version":1,"note":null,"platform":null})json";
        } else if (state == "json_full") {
            payload =
                R"json({"id":101,"version":1,"name":"VISIBLE-NAME","note":"VISIBLE-NOTE","platform_id":100})json";
        } else if (state == "json_relation_missing_name") {
            payload = R"json({"id":101,"version":1,"platform":{"id":100,"version":1}})json";
        } else if (state == "json_relation_named") {
            payload =
                R"json({"id":101,"version":1,"platform":{"id":100,"version":1,"name":"PLATFORM-NAME"}})json";
        }
        auto json = Json::parse(r, payload).value();
        auto result = user_context.decode_json_entity<T>(json);
        if (!result) {
            detail::model_put(output, "prepared", false);
            detail::model_put(output, "error", result.error().to_string(r));
            detail::model_put(output, "debug", debug_string(r, result.error()));
            return output;
        }
        entity = &user_context.make<T>(std::move(result).value());
    } else {
        entity = &std::move(request)
                      .comment("prepare expression identity")
                      .purpose("qualify explicit expression reads")
                      .newEntity(user_context);
        if (state == "new_name") {
            entity->updateName("VISIBLE-NAME");
        }
        if (state == "new_note_null") {
            entity->updateNote(std::nullopt);
        }
    }
    auto expression = factory(*entity);
    Json result(r);
    if (path == "id") {
        result = read(expression.getId(), action);
    } else if (path == "name") {
        result = read(expression.getName(), action);
    } else if (path == "note") {
        result = read(expression.getNote(), action);
    } else if (path == "version") {
        result = read(expression.getVersion(), action);
    } else if (path == "platform_id") {
        result = read(expression.getPlatformId(), action);
    } else if (path == "platform_name") {
        result = read(expression.getPlatform().getName(), action);
    } else {
        throw std::logic_error("Unknown fixture path");
    }
    detail::model_put(output, "prepared", true);
    detail::model_put(output, "identity", T::ENTITY_NAME);
    detail::model_put(output, "id", entity->id());
    detail::model_put(output, "read", std::move(result));
    return output;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        {
            domain::module(user_context).apply_to(user_context);
            auto& ids = user_context.make_resource<Ids>();
            user_context.set_internal_id_generator(user_context, ids);
            auto r = user_context.memory_resource();
            auto input = Json::parse(r, line).value();
            auto Q = user_context.queries<domain::query_factory>();
            auto output = text(field(input, "entity")) == "item"
                              ? inspect<domain::Item>(
                                    user_context,
                                    Q.items(),
                                    [](const domain::Item& value) {
                                        return domain::E::item(value);
                                    },
                                    input)
                              : inspect<domain::Child>(
                                    user_context,
                                    Q.children(),
                                    [](const domain::Child& value) {
                                        return domain::E::child(value);
                                    },
                                    input);
            detail::model_put(output, "sql_count", user_context.sql_logs().size());
            std::cout << output.dump(r) << '\n';
        }
        owner.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Expression identity probe retains UserContext memory");
        }
    }
}
