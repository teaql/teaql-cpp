#include <teaql/core/eval.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;
using namespace teaql::eval;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Company {
    std::optional<std::string_view> name;
    LoadState state;

    EvalResult<std::string_view> eval_name(Memory owner) const {
        if (!state.is_loaded("name")) {
            return EvalResult<std::string_view>::not_loaded(owner, "name", "name");
        }
        return name ? EvalResult<std::string_view>::value(owner, *name)
                    : EvalResult<std::string_view>::null(owner);
    }
};

struct Platform {
    const Company* company;
    LoadState state;

    EvalResult<const Company*> eval_company(Memory owner) const {
        if (!state.is_loaded("company")) {
            return EvalResult<const Company*>::not_loaded(owner, "company", "company");
        }
        return company ? EvalResult<const Company*>::value(owner, company)
                       : EvalResult<const Company*>::null(owner);
    }
};

struct User {
    const Platform* platform;
    LoadState state;

    EvalResult<const Platform*> eval_platform(Memory owner) const {
        if (!state.is_loaded("platform")) {
            return EvalResult<const Platform*>::not_loaded(owner, "platform", "platform");
        }
        return platform ? EvalResult<const Platform*>::value(owner, platform)
                        : EvalResult<const Platform*>::null(owner);
    }
};

static auto evaluate(Memory owner, const User& user) {
    return user.eval_platform(owner).and_then("platform", [&](const Platform* platform) {
        return platform->eval_company(owner).and_then("company", [&](const Company* company) {
            return company->eval_name(owner);
        });
    });
}

static void original_tracking_contracts(Memory owner) {
    Company missing_name{{}, LoadState::not_loaded(owner)};
    Platform loaded_company{&missing_name, LoadState::fully_loaded(owner)};
    User first{&loaded_company, LoadState::fully_loaded(owner)};
    auto result = evaluate(owner, first);
    check(result.is_not_loaded() && result.missing().attempted_path == "platform.company.name" &&
              result.missing().failed_node == "name",
          "original perfect path retains the missing leaf and complete path");

    Platform missing_company{nullptr, LoadState::not_loaded(owner)};
    User second{&missing_company, LoadState::fully_loaded(owner)};
    auto middle = evaluate(owner, second);
    check(middle.is_not_loaded() && middle.missing().attempted_path == "platform.company" &&
              middle.missing().failed_node == "company",
          "original middle break does not append an unevaluated leaf");

    Company null_name{{}, LoadState::fully_loaded(owner)};
    Platform null_company{&null_name, LoadState::fully_loaded(owner)};
    User third{&null_company, LoadState::fully_loaded(owner)};
    check(evaluate(owner, third).is_null(), "original loaded null differs from NotLoaded");
}

static void ownership_and_layouts() {
    GlobalContext global;
    Ref<LoadState> retained_state;
    Ref<EvalResult<String>> retained_result;
    const ColumnLayout* original_layout = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        original_tracking_contracts(owner);
        std::array<std::string_view, 3> names{"name", "name", "dynamic_projection_alias_long"};
        Vector<String> columns(owner);
        for (auto name : names) {
            columns.emplace_back(name);
        }
        auto layout = user->make_ref<ColumnLayout>(std::move(columns));
        original_layout = &user->get(layout);
        auto state = user->make_ref<LoadState>(LoadState::shared_columns(owner, *original_layout));
        auto same = user->make_ref<LoadState>(user->get(state));
        check(user->get(same).column_layout() == original_layout &&
                  user->get(state).names().size() == 3,
              "same-domain clones share immutable result-set layout and preserve duplicates");
        retained_state = user->make_global<LoadState>(user->get(state));
        check(user->get(retained_state).column_layout() != original_layout,
              "explicit Global retention deep-copies shared layout");
        auto partial = LoadState::partial(owner, names);
        auto compact = LoadState::partial_compact(owner, names);
        check(partial.names().size() == 2 && compact.names().size() == 3 && partial != compact,
              "set and compact states keep distinct equality and duplicate semantics");
        auto other = global.make_user_context();
        bool rejected = false;
        try {
            (void)LoadState::shared_columns(other->memory_resource(), *original_layout);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "sibling Context cannot retain User layout storage");
        auto failed = user->make_ref<EvalResult<String>>(EvalResult<String>::not_loaded(
            owner, String(4096, 'n', owner), String(8192, 'p', owner)));
        retained_result = user->make_global<EvalResult<String>>(user->get(failed));
    }
    auto next = global.make_user_context();
    auto& state = next->get(next->rebind_global(retained_state));
    auto& result = next->get(next->rebind_global(retained_result));
    check(state.kind() == LoadState::Kind::SharedColumns &&
              state.is_loaded("dynamic_projection_alias_long") && !state.is_loaded("unknown") &&
              state.column_layout()->memory_resource() == global.memory_resource(),
          "retained layout remains complete after actual source User deletion");
    for (const auto& name : state.names()) {
        check(name.get_allocator().resource() == global.memory_resource(),
              "shared layout characters belong to destination");
    }
    check(result.is_not_loaded() && result.missing().failed_node.size() == 4096 &&
              result.missing().attempted_path.size() == 8192 &&
              result.missing().attempted_path.get_allocator().resource() ==
                  global.memory_resource(),
          "failed-path strings retain their explicit destination lifetime");
    auto borrowed_global =
        LoadState::shared_columns(next->memory_resource(), *state.column_layout());
    check(borrowed_global.column_layout() == state.column_layout(),
          "a live User may borrow the longer-lived Global layout");
}

struct OwnedOnly {
    using allocator_type = Allocator;
    Memory owner;
    std::unique_ptr<int> value;

    OwnedOnly(Memory owner, int value) : owner(owner), value(std::make_unique<int>(value)) {}

    OwnedOnly(std::allocator_arg_t, Allocator a, OwnedOnly&& other) : owner(a.resource()) {
        if (owner != other.owner) {
            throw std::invalid_argument("Move-only resource cannot change Context");
        }
        value = std::move(other.value);
    }

    OwnedOnly(OwnedOnly&&) noexcept = default;
    OwnedOnly(const OwnedOnly&) = delete;

    Memory memory_resource() const noexcept {
        return owner;
    }
};

static_assert(!detail::domain_copyable<EvalResult<OwnedOnly>>);

static void consuming_callbacks(Memory owner) {
    int calls = 0;
    auto output =
        EvalResult<OwnedOnly>::value(owner, OwnedOnly(owner, 7)).map([&](OwnedOnly value) {
            ++calls;
            return *value.value + 1;
        });
    check(calls == 1 && output.get() == 8, "map consumes a move-only payload without Clone");
    auto never = [&](int) {
        ++calls;
        return EvalResult<String>::value(owner, String("unexpected", owner));
    };
    auto null = EvalResult<int>::null(owner).and_then("ignored", never);
    auto missing = EvalResult<int>::not_loaded(owner, "node", "path").and_then("ignored", never);
    check(calls == 1 && null.is_null() && missing.missing().attempted_path == "path",
          "Null and NotLoaded short circuit callbacks and preserve the original path");
    auto mapped = EvalResult<int>::not_loaded(owner, "node", "path").map([&](int) {
        ++calls;
        return 4;
    });
    check(calls == 1 && mapped.missing().failed_node == "node" &&
              mapped.missing().attempted_path == "path",
          "map preserves missing diagnostics without evaluating");
    for (auto path : {"", "field", "leaf"}) {
        auto result = EvalResult<int>::value(owner, 1).and_then("field", [&](int) {
            return EvalResult<String>::not_loaded(owner, "node", path);
        });
        check(result.missing().attempted_path ==
                  (std::string_view(path) == "leaf" ? "field.leaf" : "field"),
              "and_then handles empty, equal and nested attempted paths");
    }
}

static void generic_domains_and_codecs() {
    GlobalContext global;
    Ref<EvalResult<Vector<String>>> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto foreign = global.make_user_context();
        auto other = foreign->memory_resource();
        auto nested = EvalResult<int>::value(owner, 42).and_then("ignored", [&](int value) {
            Vector<String> values(other);
            values.emplace_back(4096, 'x');
            values.emplace_back(std::to_string(value));
            return EvalResult<Vector<String>>::value(other, std::move(values));
        });
        check(nested.memory_resource() == owner &&
                  nested.get().get_allocator().resource() == owner &&
                  nested.get()[0].get_allocator().resource() == owner && nested.get()[1] == "42",
              "and_then relocates a foreign nested payload to the input result domain");
        retained = user->make_global<EvalResult<Vector<String>>>(nested);
        auto missing = EvalResult<int>::value(owner, 42).and_then("parent", [&](int) {
            return EvalResult<int>::not_loaded(other, "leaf", "child");
        });
        check(missing.memory_resource() == owner &&
                  missing.missing().attempted_path == "parent.child" &&
                  missing.missing().attempted_path.get_allocator().resource() == owner &&
                  missing.missing().failed_node.get_allocator().resource() == owner,
              "foreign missing diagnostics are relocated before path composition");
        auto moved_once = EvalResult<int>::value(owner, 7).map(
            [payload = std::make_unique<int>(5)](int value) mutable {
                auto owned = std::move(payload);
                return value + *owned;
            });
        check(moved_once.get() == 12, "map accepts a noncopyable consuming callback");

        int encoded = 0;
        auto encode = [&](const int& value) {
            ++encoded;
            return Json(other, value);
        };
        auto json = EvalResult<int>::value(owner, 42).to_json(owner, encode);
        check(encoded == 1 && json.memory_resource() == owner &&
                  json.dump(owner) == "{\"Value\":42}",
              "generic encoder runs only for Value and returns destination-owned JSON");
        (void)EvalResult<int>::null(owner).to_json(owner, encode);
        (void)EvalResult<int>::not_loaded(owner, "node", "path").to_json(owner, encode);
        check(encoded == 1, "Null and NotLoaded do not invoke the generic encoder");
        int decoded = 0;
        auto decode = [&](const Json& value) {
            ++decoded;
            return detail::model_integer<int>(value, "Value");
        };
        auto parsed = EvalResult<int>::from_json(owner, json, decode);
        check(parsed && parsed.value().get() == 42 && decoded == 1,
              "generic decoder preserves a non-string Value payload");
        for (auto text :
             {"\"Null\"",
              "{\"NotLoaded\":{\"failed_node\":\"node\",\"attempted_path\":\"path\"}}"}) {
            auto input = Json::parse(owner, text);
            check(input.has_value(), "valid generic evaluation fixture JSON");
            auto result = EvalResult<int>::from_json(owner, *input, decode);
            check(bool(result), "generic unit/struct decode succeeds");
        }
        check(decoded == 1, "non-Value variants do not invoke generic decoding");
        auto invalid = Json::parse(owner, "{\"Value\":\"wrong integer\"}");
        check(invalid.has_value(), "valid malformed-payload fixture JSON");
        auto failure = EvalResult<int>::from_json(owner, *invalid, decode);
        check(!failure && decoded == 2 &&
                  failure.error().code == ModelDecodeError::Code::ExpectedInteger,
              "generic payload decoder failure propagates without manufacturing Value");
    }
    check(global.live_user_bytes() == 0, "generic outputs retain no source User blocks");
    auto next = global.make_user_context();
    const auto& value = next->get(next->rebind_global(retained));
    check(value.get().size() == 2 && value.get()[0].size() == 4096 && value.get()[1] == "42" &&
              value.get()[0].get_allocator().resource() == global.memory_resource(),
          "explicitly retained generic nested output survives actual deletion of both Users");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        Memory previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    try {
        ownership_and_layouts();
        generic_domains_and_codecs();
        {
            GlobalContext global;
            auto user = global.make_user_context();
            consuming_callbacks(user->memory_resource());
        }
        std::cout << "Evaluation ownership and original tracking contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
