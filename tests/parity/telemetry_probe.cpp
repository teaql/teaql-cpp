#include <teaql/runtime/context.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
#include <charconv>
using namespace teaql;

struct Shared {
    Vector<String> notes;
    bool active = false;

    explicit Shared(Memory r) : notes(r) {}

    void note(std::string_view s) {
        notes.emplace_back(s);
    }
};

static Json attribute_json(Memory r, const RuntimeAttributeValue& value) {
    auto j = detail::model_json(r);
    std::visit(
        [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, String>) {
                detail::model_put(j, "kind", "string");
                detail::model_put(j, "value", v);
            } else if constexpr (std::same_as<T, bool>) {
                detail::model_put(j, "kind", "bool");
                detail::model_put(j, "value", v);
            } else {
                char b[40];
                if constexpr (std::same_as<T, double>) {
                    detail::model_put(j, "kind", "float");
                    auto end = std::to_chars(b, b + 40, std::bit_cast<std::uint64_t>(v), 16).ptr;
                    String s(16 - (end - b), '0', r);
                    s.append(b, end);
                    detail::model_put(j, "value", s);
                } else {
                    detail::model_put(j, "kind", "integer");
                    auto end = std::to_chars(b, b + 40, v).ptr;
                    detail::model_put(j, "value", std::string_view(b, end - b));
                }
            }
        },
        value.data);
    return j;
}

static Json model(Memory r, const Json& j) {
    RuntimeOperation op(
        std::allocator_arg, Allocator(r), text(field(j, "family")), text(field(j, "name")));
    for (const auto& a : std::get<Json::Array>(field(j, "attributes").data)) {
        auto kind = text(field(a, "kind")), key = text(field(a, "key"));
        const auto& v = field(a, "value");
        if (kind == "string")
            op.attribute(key, text(v));
        else if (kind == "bool")
            op.attribute(key, std::get<bool>(v.data));
        else if (kind == "u64")
            op.attribute(key, number<std::uint64_t>(v));
        else if (kind == "float") {
            std::uint64_t bits = 0;
            auto s = text(v);
            std::from_chars(s.data(), s.data() + s.size(), bits, 16);
            op.attribute(key, std::bit_cast<double>(bits));
        } else
            op.attribute(key, number<std::int64_t>(v));
    }
    auto out = detail::model_json(r), attrs = detail::model_json(r);
    for (const auto& [k, v] : op.attributes)
        detail::model_put(attrs, k, attribute_json(r, v));
    detail::model_put(out, "family", op.family);
    detail::model_put(out, "name", op.name);
    detail::model_put(out, "attributes", attrs);
    Json::Array categories(r);
    for (const auto& e : std::get<Json::Array>(field(j, "errors").data))
        categories.emplace_back(runtime_error_category(text(e)));
    detail::model_put(out, "categories", categories);
    return out;
}

static void activate(Shared& state, std::string_view behavior, FunctionRef<void()> callback) {
    state.note("enter");
    state.active = true;

    struct Guard {
        Shared& state;

        ~Guard() {
            state.active = false;
            state.note("leave");
        }
    } guard{state};

    if (behavior == "before")
        throw std::runtime_error("adapter");
    if (behavior == "skip")
        return;
    if (behavior == "swallow" || behavior == "swallow_after") {
        try {
            callback();
        } catch (...) {
        }
        if (behavior == "swallow_after")
            throw std::runtime_error("adapter");
        return;
    }
    callback();
    if (behavior == "twice")
        callback();
    if (behavior == "after")
        throw std::runtime_error("adapter");
}

struct Scope final : RuntimeTelemetryScope {
    Shared& state;
    std::string_view behavior;
    bool fail;

    Scope(Shared& s, std::string_view b, bool f) : state(s), behavior(b), fail(f) {}

    ~Scope() {
        state.note("scope.drop");
    }

    void with_context(FunctionRef<void()> cb) const override {
        activate(state, behavior, cb);
    }

    void success(const RuntimeAttributes& attrs) override {
        state.note("success");
        if (fail)
            throw std::runtime_error("finish");
    }

    void failure(std::string_view type) override {
        state.note(type);
        if (fail)
            throw std::runtime_error("finish");
    }
};

struct Prop final : RuntimeTelemetryPropagationContext {
    Shared& state;
    std::string_view behavior;

    Prop(Shared& s, std::string_view b) : state(s), behavior(b) {}

    ~Prop() {
        state.note("prop.drop");
    }

    void with_context(FunctionRef<void()> cb) const override {
        activate(state, behavior, cb);
    }
};

struct Adapter final : RuntimeTelemetry {
    using allocator_type = Allocator;
    Memory r;
    Shared& state;
    std::string_view behavior;
    bool create_fail, finish_fail;

    Adapter(std::allocator_arg_t, Allocator a, Shared& s, std::string_view b, bool c, bool f)
        : r(a.resource()), state(s), behavior(b), create_fail(c), finish_fail(f) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory r, const RuntimeOperation&) const override {
        state.note("start");
        if (create_fail)
            throw std::runtime_error("create");
        return detail::resource_arena(r).make_resource<ContextBox<RuntimeTelemetryScope>>(
            std::in_place_type<Scope>, state, behavior, finish_fail);
    }

    ContextBox<RuntimeTelemetryPropagationContext>&
    extract_context(Memory r, const RuntimeContextCarrier&) const override {
        state.note("extract");
        if (create_fail)
            throw std::runtime_error("create");
        return detail::resource_arena(r)
            .make_resource<ContextBox<RuntimeTelemetryPropagationContext>>(
                std::in_place_type<Prop>, state, behavior);
    }
};

template <class S>
static Json runs(Memory r, S& scope, Shared& state, const Json& action) {
    Json::Array results(r);
    unsigned count = 0;
    auto& sequence = std::get<Json::Array>(field(action, "sequence").data);
    auto future = [&]() -> std::optional<unsigned> {
        auto index = count++;
        String note("poll:", state.notes.get_allocator().resource());
        note += std::to_string(count);
        note += state.active ? ":inside" : ":outside";
        state.note(note);
        auto value = text(sequence[std::min<std::size_t>(index, sequence.size() - 1)]);
        if (value == "panic")
            throw std::runtime_error("business");
        if (value == "pending")
            return {};
        return count;
    };
    for (unsigned i = 0; i < number<unsigned>(field(action, "polls")); ++i) {
        try {
            auto v = scope.poll_with_context(future);
            if (v) {
                auto j = detail::model_json(r);
                detail::model_put(j, "ready", std::to_string(*v));
                results.emplace_back(std::move(j));
                break;
            }
            results.emplace_back();
        } catch (const std::exception& e) {
            auto j = detail::model_json(r);
            detail::model_put(j, "panic", e.what());
            results.emplace_back(std::move(j));
            break;
        }
    }
    state.note("future.drop");
    return Json(r, results);
}

static Json flow(GlobalContext& global, const Json& input) {
    auto g = global.memory_resource();
    Shared state(g);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    bool prop = std::get<bool>(field(input, "propagation").data),
         noop = std::get<bool>(field(input, "noop").data);
    if (!noop) {
        auto& adapter =
            user->make_resource<Adapter>(state,
                                         text(field(input, "behavior")),
                                         std::get<bool>(field(input, "create_fail").data),
                                         std::get<bool>(field(input, "finish_fail").data));
        user->set_runtime_telemetry(*user, adapter);
    }
    bool is_noop = user->runtime_telemetry_is_noop();
    Json::Array results(g);
    if (prop) {
        RuntimeContextCarrier carrier(r);
        auto& scope = extract_runtime_context(r, user->runtime_telemetry(), carrier);
        for (const auto& action : std::get<Json::Array>(field(input, "actions").data))
            if (text(field(action, "op")) == "run")
                results.emplace_back(runs(g, scope, state, action));
    } else {
        RuntimeOperation operation(std::allocator_arg, Allocator(r), "query", "Order.list");
        auto& scope = user->start_runtime_operation(operation);
        for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
            auto op = text(field(action, "op"));
            if (op == "run")
                results.emplace_back(runs(g, scope, state, action));
            else if (op == "success") {
                RuntimeAttributes attrs(r);
                scope.success(attrs);
            } else
                scope.failure("native_error");
        }
    }
    user.reset();
    auto out = detail::model_json(g);
    Json::Array notes(g);
    for (const auto& n : state.notes)
        notes.emplace_back(n);
    detail::model_put(out, "notes", notes);
    detail::model_put(out, "results", results);
    detail::model_put(out, "noop", is_noop);
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto r = global.memory_resource();
        auto input = Json::parse(r, line);
        std::cout << (text(field(*input, "mode")) == "model" ? model(r, *input)
                                                             : flow(global, *input))
                         .dump(r)
                  << '\n';
    }
}
