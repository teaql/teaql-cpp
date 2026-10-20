#include "../generated_typed_member_aliases/include/cpp_member_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>

using namespace teaql;

template <Entity T>
static Result<T, EntityError> decode(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Vector<String> names(r);
    names.emplace_back("id");
    names.emplace_back("version");
    Vector<Value> values(r);
    values.emplace_back(std::uint64_t{1});
    values.emplace_back(std::int64_t{1});
    for (const auto& alias : std::get<Json::Array>(field(input, "names").data)) {
        const auto& name = std::get<String>(alias.data);
        names.emplace_back(name);
        if (std::get<bool>(field(input, "null").data)) {
            values.emplace_back(nullptr);
        } else {
            values.emplace_back(name);
        }
    }
    auto& columns = user_context.make<ColumnLayout>(std::move(names));
    return T::from_compact_row(
        r, CompactRow(std::allocator_arg, Allocator(r), columns, std::move(values)));
}

template <Entity T>
static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto result = decode<T>(user_context, input);
    auto out = detail::model_json(r);
    if (!result) {
        detail::model_put(out, "error", result.error().to_string(r));
        return out;
    }
    const auto& entity = result.value();
    detail::model_put(out, "value", entity.type() ? Json(r, *entity.type()) : Json(r));
    auto loaded = detail::model_json(r);
    for (auto name : {"id", "version", "type", "type_field", "legacy_type", "type_typo"}) {
        detail::model_put(loaded, name, entity.is_field_loaded(name));
    }
    detail::model_put(out, "loaded", loaded);
    detail::model_put(out, "dirty", entity.dirty_fields().has_value());
    detail::model_put(out, "new", entity.is_new());
    return out;
}

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void self_test() {
    GlobalContext global;
    Ref<cpp_member_aliases::Record> retained;
    Ref<cpp_member_aliases::Record> expired;
    std::optional<cpp_member_aliases::RecordExpression> expression;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        for (auto name : {"type", "type_field", "legacy_type", "type_typo"}) {
            auto input =
                Json::parse(r, std::string("{\"names\":[\"") + name + "\"],\"null\":false}");
            auto ordinary = decode<cpp_member_aliases::Record>(*user_context, *input);
            auto inherited = decode<cpp_member_aliases::ChildRecord>(*user_context, *input);
            check(ordinary && inherited,
                  "Ordinary and inherited actual generated decoders succeed");
            const bool known = std::string_view(name) != "type_typo";
            check(ordinary.value().type().has_value() == known &&
                      inherited.value().type().has_value() == known,
                  "Generated decoders must materialize every known alias");
            if (known) {
                check(*ordinary.value().type() == name && *inherited.value().type() == name,
                      "Generated ordinary/inherited storage holds the actual alias payload");
            }
        }
        {
            Record child(r);
            child.emplace(String("id", r), Value(r, std::uint64_t{7}));
            child.emplace(String("version", r), Value(r, std::int64_t{1}));
            child.emplace(String("type_field", r), Value(r, String(512, 'x', r)));
            Record parent(r);
            parent.emplace(String("id", r), Value(r, std::uint64_t{8}));
            parent.emplace(String("version", r), Value(r, std::int64_t{1}));
            parent.emplace(String("record", r), Value(r, std::move(child)));
            auto holder = cpp_member_aliases::Holder::from_compact_row(
                r, CompactRow(std::allocator_arg, Allocator(r), std::move(parent)));
            check(holder.is_ok(), "Forward generated decoder retains the reference object rules");
            auto value =
                cpp_member_aliases::E::holder(holder.value()).getRecord().getType().unwrap();
            check(value && value->size() == 512,
                  "Nested generated decode uses the same metadata-aware alias path");
        }
        {
            auto input = Json::parse(r, "{\"names\":[\"type_field\"],\"null\":false}");
            auto entity = decode<cpp_member_aliases::Record>(*user_context, *input);
            check(entity.is_ok(), "Lifetime fixture decodes through generated adapter");
            expired = user_context->make_ref<cpp_member_aliases::Record>(std::move(entity).value());
            expression.emplace(cpp_member_aliases::E::record(user_context->get(expired)));
            retained =
                user_context->make_global<cpp_member_aliases::Record>(user_context->get(expired));
        }
    }
    bool rejected = false;
    try {
        (void)expression->getType().unwrap();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected && global.live_user_bytes() == 0,
          "Generated E borrows fail after actual User deletion without retaining its arena");
    auto next_user_context = global.make_user_context();
    const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    check(copy.type() && *copy.type() == "type_field" && copy.is_field_loaded("legacy_type"),
          "Explicit Global copy preserves the decoded payload and its shared availability");
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            self_test();
        } else {
            std::string line;
            while (std::getline(std::cin, line)) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto input = Json::parse(r, line);
                auto output = std::get<String>(field(*input, "model").data) == "record"
                                  ? observe<cpp_member_aliases::Record>(*user_context, *input)
                                  : observe<cpp_member_aliases::ChildRecord>(*user_context, *input);
                std::cout << output.dump(r) << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
