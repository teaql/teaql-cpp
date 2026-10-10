#include <teaql/runtime/detail/data_service/id_set_page.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include <iostream>

using namespace teaql;

static const Json& field(const Json& input, std::string_view name) {
    return std::get<Json::Object>(input.data).find(name)->second;
}

static std::uint64_t number(const Json& input, std::string_view name) {
    return std::get<std::uint64_t>(field(input, name).data);
}

static bool flag(const Json& input, std::string_view name) {
    return std::get<bool>(field(input, name).data);
}

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T value;

    Ready(std::allocator_arg_t, Allocator a, T value)
        : value(detail::move_in(a.resource(), std::move(value))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(value);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Store : IdSetStore {
    using allocator_type = Allocator;
    Memory resource;
    Vector<String> notes;
    std::optional<RetainedIdSet> value;
    std::size_t reads = 0, fail_read;
    bool fail_put;

    Store(std::allocator_arg_t, Allocator a, std::size_t fail_read, bool fail_put)
        : resource(a.resource()), notes(resource), fail_read(fail_read), fail_put(fail_put) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>&
    get(Memory r, std::string_view key) override {
        String note("get:", resource);
        note += key;
        notes.emplace_back(std::move(note));
        using R = PaginationResult<std::optional<RetainedIdSet>>;
        if (++reads == fail_read) {
            return make_operation<R, Ready<R>>(r, R::err(r, String("store read failed", r)));
        }
        auto retained =
            value && value->query_key == key ? detail::clone_in(r, value) : std::nullopt;
        return make_operation<R, Ready<R>>(r, R::ok(r, std::move(retained)));
    }

    AsyncOperation<PaginationWriteResult>& put(Memory r, const RetainedIdSet& ids) override {
        String note("put:", resource);
        note += ids.query_key;
        notes.emplace_back(std::move(note));
        using R = PaginationWriteResult;
        if (fail_put) {
            return make_operation<R, Ready<R>>(r, R::err(r, String("store write failed", r)));
        }
        value.emplace(std::allocator_arg, Allocator(resource), ids);
        return make_operation<R, Ready<R>>(r, R::ok(r, {}));
    }

    AsyncOperation<PaginationWriteResult>& invalidate(Memory r, std::string_view) override {
        return make_operation<PaginationWriteResult, Ready<PaginationWriteResult>>(
            r, PaginationWriteResult::ok(r, {}));
    }
};

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    Json::Array ids;
    Vector<String> requests;
    bool fail;

    Mock(std::allocator_arg_t, Allocator a, const Json::Array& ids, bool fail)
        : resource(a.resource()), ids(ids, resource), requests(resource), fail(fail) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        requests.emplace_back(detail::debug_query(resource, request.query));
        using R = Result<QueryResult, String>;
        if (fail) {
            return make_operation<R, Ready<R>>(r, R::err(r, String("ID lookup failed", r)));
        }
        QueryResult result(
            std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, ids.size()));
        for (const auto& id : ids) {
            result.rows.emplace_back();
            if (auto* number = std::get_if<std::uint64_t>(&id.data)) {
                result.rows.back().insert("id", Value(r, *number));
            } else if (auto* text = std::get_if<String>(&id.data)) {
                result.rows.back().insert("id", Value(r, *text));
            }
        }
        return make_operation<R, Ready<R>>(r, R::ok(r, std::move(result)));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

static Json strings(Memory r, const Vector<String>& source) {
    Json::Array out(r);
    for (const auto& value : source) {
        out.emplace_back(value);
    }
    return Json(r, out);
}

static Json evaluate(UserContext& context, const Json& input) {
    auto r = context.memory_resource();
    context.set_user_identifier("reader");
    if (!flag(input, "capture")) {
        context.disable_sql_log();
    }
    if (auto* root = std::get_if<Json::Array>(&field(input, "root").data)) {
        context.with_active_root(std::get<String>((*root)[0].data),
                                 std::get<std::uint64_t>((*root)[1].data));
    }
    const auto& fields = std::get<Json::Object>(input.data);
    if (auto controls = fields.find("root_controls"); controls != fields.end()) {
        Json::Array results(r);
        for (const auto& command : std::get<Json::Array>(controls->second.data)) {
            const auto& parts = std::get<Json::Array>(command.data);
            const auto& entity = std::get<String>(parts[1].data);
            auto out = detail::model_json(r);
            if (std::get<String>(parts[0].data) == "bootstrap") {
                auto result = context.set_generated_bootstrap_active_root(
                    entity, std::get<std::uint64_t>(parts[2].data));
                if (result) {
                    detail::model_put(out, "ok", true);
                } else {
                    detail::model_put(out, "error", result.error().to_string(r));
                }
            } else {
                auto result = context.require_active_root(entity);
                if (result) {
                    detail::model_put(out, "entity", result.value().get().entity_type);
                    detail::model_put(out, "id", result.value().get().id);
                } else {
                    detail::model_put(out, "error", result.error().to_string(r));
                }
            }
            results.emplace_back(std::move(out));
        }
        return Json(r, results);
    }
    auto& store = context.make<Store>(number(input, "fail_read"), flag(input, "fail_put"));
    context.set_id_set_store(context, store);
    auto& executor =
        context.make<Mock>(std::get<Json::Array>(field(input, "ids").data), flag(input, "failure"));
    auto& repo = EntityDataService<Mock>::for_executor(context, "Item", context, executor);
    auto query = repo.select();
    query.project("label").comment("purpose");
    if (flag(input, "enabled")) {
        query.optimize_pagination_with_id_set_config("fixture", 600, number(input, "max_ids"));
    }
    if (auto* limit = std::get_if<std::uint64_t>(&field(input, "limit").data)) {
        query.page(number(input, "offset"), *limit);
    }
    const auto& shape = std::get<String>(field(input, "shape").data);
    if (shape == "aggregate") {
        query.count("count");
    } else if (shape == "grouped") {
        query.group_by("id");
    } else if (shape == "continuous") {
        query.optimize_for_continuous_page_fetch();
    } else if (shape == "raw") {
        query.raw_sql("SELECT id FROM items");
    } else if (shape == "partition") {
        query.partition_by("owner");
    } else if (shape == "having") {
        query.having(Expr::eq(r, "id", std::uint64_t(1)));
    } else if (shape == "expr_order") {
        query.order_expr_asc(Expr::value(r, std::uint64_t(1)));
    } else if (shape == "desc") {
        query.order_desc("id");
    } else if (shape == "tie") {
        query.order_desc("label");
    }
    Json::Array results(r);
    for (std::uint64_t i = 0; i != number(input, "calls"); ++i) {
        auto& result = detail::prepare_id_set_page(repo, query).wait();
        auto out = detail::model_json(r);
        if (result) {
            detail::model_put(out, "query", detail::debug_query(r, result.value().query));
            detail::model_put(out,
                              "total",
                              result.value().total_count ? Json(r, *result.value().total_count)
                                                         : Json(r));
        } else {
            detail::model_put(out, "error", result.error().to_string(r));
        }
        results.emplace_back(std::move(out));
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "results", results);
    detail::model_put(output, "requests", strings(r, executor.requests));
    detail::model_put(output, "store", strings(r, store.notes));
    auto plan = context.id_set_plan();
    auto count = context.id_set_count();
    detail::model_put(output, "plan", plan ? Json(r, *plan) : Json(r));
    detail::model_put(output, "count", count ? Json(r, *count) : Json(r));
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("invalid fixture JSON");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
