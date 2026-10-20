#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <iostream>

using namespace teaql;

static DynamicFieldDefinitions&
definitions(const UserContext& user_context, std::string_view owner, std::string_view revision) {
    const std::array fields{DynamicFieldDefinitions::Field{"a", DataType::Text},
                            DynamicFieldDefinitions::Field{"b", DataType::I64}};
    return user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(
                      user_context.memory_resource(), owner, revision, fields))
            .value());
}

static DynamicFieldSelection selection(Memory r, std::string_view mode) {
    if (mode == "all") {
        return DynamicFieldSelection::all(r);
    }
    Vector<DynamicFieldSelection::Field> fields(r);
    if (mode == "a") {
        fields.emplace_back("a", DataType::Text);
    } else if (mode == "b") {
        fields.emplace_back("b", DataType::I64);
    } else if (mode == "wrong_type") {
        fields.emplace_back("a", DataType::I64);
    } else if (mode == "unknown") {
        fields.emplace_back("missing", DataType::Text);
    } else if (mode != "empty") {
        throw std::logic_error("fixture selection");
    }
    return std::move(DynamicFieldSelection::fields(r, fields)).value();
}

static std::optional<Value> identity(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    if (kind == "missing") {
        return {};
    }
    if (kind == "u64") {
        return Value(r, detail::model_integer<std::uint64_t>(field(input, "value"), "id"));
    }
    if (kind == "null") {
        return Value(r);
    }
    if (kind == "negative") {
        return Value(r, std::int64_t(-1));
    }
    if (kind == "text") {
        return Value(r, "1");
    }
    if (kind == "bool") {
        return Value(r, true);
    }
    if (kind == "typed_null") {
        return Value(r, TypedNull{DataType::U64});
    }
    if (kind == "f64") {
        return Value(r, 1.0);
    }
    if (kind == "i64_one") {
        return Value(r, std::int64_t(1));
    }
    throw std::logic_error("fixture id");
}

static Value::Object payload(Memory r, const Json& input, std::uint64_t id) {
    Value::Object values(r);
    auto batch = text(field(input, "batch"));
    auto mode = text(field(input, "payload"));
    if (batch == "empty_payload" || mode == "empty") {
        return values;
    }
    if (batch == "wrong_value") {
        values.emplace(String("a", r), Value(r, std::int64_t(7)));
        return values;
    }
    if (batch == "unselected" || (mode == "split" && id == 2)) {
        values.emplace(String("b", r), Value(r, std::int64_t(4)));
        return values;
    }
    if (mode == "null" && id == 1) {
        values.emplace(String("a", r), Value(r));
    } else {
        char buffer[32];
        auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), id);
        if (error != std::errc{}) {
            throw std::logic_error("fixture label");
        }
        String label("new", r);
        label.append(buffer, end);
        values.emplace(String("a", r), Value(r, label));
    }
    return values;
}

struct State {
    using allocator_type = Allocator;
    Json input;
    Json::Array events;
    DynamicFieldSelection selected;

    State(std::allocator_arg_t, Allocator a, const Json& input)
        : input(std::allocator_arg, a, input), events(a.resource()),
          selected(selection(a.resource(), text(field(input, "selection")))) {}
};

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T result;

    Ready(std::allocator_arg_t, Allocator a, T result)
        : result(detail::move_in(a.resource(), std::move(result))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(result);
    }

    void cancel_and_wait() noexcept {}

    void finish_and_wait() noexcept {}
};

struct Provider final : DynamicFieldsProvider {
    using allocator_type = Allocator;
    State& state;
    const DynamicFieldDefinitions& base;

    Provider(std::allocator_arg_t, Allocator, State& state, const DynamicFieldDefinitions& base)
        : state(state), base(base) {}

    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext& user_context,
                                                         std::string_view owner,
                                                         std::span<const std::uint64_t> ids,
                                                         const DynamicFieldSelection& selected,
                                                         const QueryIntent& intent) const override {
        auto r = user_context.memory_resource();
        auto event = detail::model_json(r);
        Json::Array identifiers(r);
        for (auto id : ids) {
            identifiers.emplace_back(id);
        }
        detail::model_put(event, "owner", owner);
        detail::model_put(event, "ids", identifiers);
        detail::model_put(event, "selection_matches", selected == state.selected);
        detail::model_put(event, "comment", intent.comment());
        detail::model_put(event, "purpose", intent.purpose());
        state.events.emplace_back(std::move(event));
        using R = Result<DynamicFieldBatch, DynamicFieldError>;
        auto batch = text(field(state.input, "batch"));
        if (batch == "provider_error") {
            return make_operation<R, Ready<R>>(
                r, R::err(r, DynamicFieldError(r, "FIXTURE_DYNAMIC_FAILED", owner)));
        }
        DynamicStorageRows rows(r);
        for (auto id : ids) {
            rows.insert_or_assign(id, payload(r, state.input, id));
        }
        if (batch == "omitted" && !ids.empty()) {
            rows.erase(ids.back());
        }
        if (batch == "unrequested") {
            rows.emplace(999, payload(r, state.input, 999));
        }
        const auto& definition = batch == "wrong_owner" ? definitions(user_context, "Other", "v1")
                                 : batch == "wrong_revision"
                                     ? definitions(user_context, "Owner", "v2")
                                     : base;
        DynamicFieldBatch result(std::allocator_arg, Allocator(r), definition, rows);
        return make_operation<R, Ready<R>>(r, R::ok(r, std::move(result)));
    }
};

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("row helper cannot fetch root");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("row helper cannot mutate");
    }
};

static Json record(Memory r, const Record& values) {
    auto out = detail::model_json(r);
    for (const auto& [name, value] : values) {
        detail::model_put(out, name, detail::debug_value(r, value));
    }
    return out;
}

static Json snapshot(Memory r, std::span<const CompactRow> rows) {
    Json::Array views(r), sharing(r);
    for (const auto& row : rows) {
        auto view = detail::model_json(r);
        detail::model_put(view, "native", record(r, row.copy_map(r)));
        const auto* loaded = row.loaded_dynamic_fields();
        if (loaded) {
            auto data = detail::model_json(r);
            detail::model_put(data, "owner", loaded->definitions().owner_type());
            detail::model_put(data, "revision", loaded->definitions().revision());
            Vector<String> codes(r);
            for (const auto& code : loaded->selected_codes()) {
                codes.emplace_back(code);
            }
            std::sort(codes.begin(), codes.end());
            Json::Array selected(r);
            for (const auto& code : codes) {
                selected.emplace_back(code);
            }
            detail::model_put(data, "selected", selected);
            detail::model_put(data, "values", record(r, loaded->values()));
            detail::model_put(view, "dynamic", data);
        } else {
            detail::model_put(view, "dynamic", Json(r));
        }
        views.emplace_back(std::move(view));
        Json::Array peers(r);
        for (const auto& other : rows) {
            const auto* right = other.loaded_dynamic_fields();
            peers.emplace_back(
                loaded && right
                    ? Json(r, loaded->selection_identity() == right->selection_identity())
                    : Json(r));
        }
        sharing.emplace_back(peers);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "rows", views);
    detail::model_put(out, "sharing", sharing);
    return out;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    user_context.disable_sql_log();
    auto& state = user_context.make<State>(input);
    auto& base = definitions(user_context, "Owner", "v1");
    auto& equal = definitions(user_context, "Owner", "v1");
    auto& old = definitions(user_context, "Owner", "v0");
    auto& other = definitions(user_context, "Other", "v1");
    auto& rows = user_context.make<Vector<CompactRow>>();
    auto mode = text(field(input, "existing"));
    for (const auto& id : std::get<Json::Array>(field(input, "ids").data)) {
        auto index = rows.size();
        Record values(r);
        values.emplace(String("tag", r), Value(r, std::uint64_t(index)));
        if (auto key = identity(r, id)) {
            values.emplace(String("id", r), std::move(*key));
        }
        rows.emplace_back(values);
        if (mode != "none") {
            const auto& definition = mode == "equal" || mode == "sparse"        ? equal
                                     : mode == "old_revision"                   ? old
                                     : mode == "other_owner"                    ? other
                                     : mode == "mixed_second_bad" && index == 1 ? old
                                                                                : base;
            Value::Object stored(r);
            if (mode == "equal" || mode == "sparse") {
                stored.emplace(String("b", r), Value(r, std::int64_t(3)));
            } else {
                stored.emplace(String("a", r), Value(r, "old"));
            }
            auto loaded = DynamicFieldValues::from_values(r, definition, stored);
            if (!loaded) {
                throw std::logic_error("fixture seed");
            }
            rows.back().set_loaded_dynamic_fields(loaded.value());
        }
    }
    auto before = snapshot(r, rows);
    if (text(field(input, "batch")) != "missing_provider") {
        auto& provider = user_context.make<Provider>(state, base);
        user_context.set_dynamic_fields_provider(user_context, provider);
    }
    auto& executor = user_context.make_resource<Executor>();
    auto& repository = EntityDataService<Executor>::for_executor(
        user_context, "Repository", user_context, executor);
    auto intent = QueryIntent::create(r, "inspect dynamic rows", "verify batch order");
    auto& work = detail::EntityDataServiceAccess::load_dynamic_fields_into_rows(
        repository, rows, "Owner", state.selected, intent.value());
    const auto& result = work.wait();
    auto outcome = detail::model_json(r);
    if (result) {
        detail::model_put(outcome, "ok", true);
    } else {
        detail::model_put(outcome, "error", result.error().to_string(r));
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "before", before);
    detail::model_put(out, "after", snapshot(r, rows));
    detail::model_put(out, "outcome", outcome);
    detail::model_put(out, "events", state.events);
    return out;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("invalid JSON");
        }
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
}
