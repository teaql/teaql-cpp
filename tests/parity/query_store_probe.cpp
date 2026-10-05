#include "../query_store_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace teaql;

static std::uint64_t number(const Json& j) {
    return std::get<std::uint64_t>(j.data);
}

static std::int64_t signed_number(const Json& j) {
    return std::holds_alternative<std::int64_t>(j.data) ? std::get<std::int64_t>(j.data)
                                                        : std::int64_t(number(j));
}

static std::span<const Json> items(const Json& j, std::string_view key) {
    auto* p = optional(j, key);
    return p ? std::span<const Json>(std::get<Json::Array>(p->data)) : std::span<const Json>{};
}

static Json strings(Memory r, const Vector<String>& names) {
    Json::Array out(r);
    for (const auto& name : names)
        out.emplace_back(name);
    return Json(r, out);
}

static Vector<CompactRow> rows_in(Memory r, const Json& j) {
    Vector<CompactRow> rows(r);
    for (const auto& input : std::get<Json::Array>(j.data)) {
        Record row(r);
        for (const auto& [key, v] : std::get<Json::Object>(input.data))
            row.emplace(String(key, r), value(r, v));
        rows.emplace_back(row);
    }
    return rows;
}

static Json rows_out(Memory r, const Vector<CompactRow>& rows) {
    Json::Array out(r);
    for (const auto& row : rows) {
        auto j = detail::model_json(r);
        for (const auto& key : row.keys())
            detail::model_put(j, key, detail::debug_value(r, *row.get(key)));
        out.emplace_back(std::move(j));
    }
    return Json(r, out);
}

static SystemTime expiry(const Json& j) {
    return SystemTime::from_unix_seconds(signed_number(field(j, "expires")),
                                         number(field(j, "nanos")));
}

static ContinuousPageCursor cursor_in(Memory r, const Json& j) {
    ContinuousPageCursor cursor(std::allocator_arg, Allocator(r));
    cursor.query_key = text(field(j, "key"));
    cursor.cursor_id = text(field(j, "cursor"));
    cursor.entity = text(field(j, "entity"));
    cursor.direction = SortDirection(number(field(j, "direction")));
    cursor.boundary = value(r, field(j, "boundary"));
    cursor.page_size = number(field(j, "page_size"));
    cursor.next_offset = number(field(j, "offset"));
    cursor.expires_at = expiry(j);
    return cursor;
}

static Json cursor_out(Memory r, const ContinuousPageCursor& c) {
    auto out = detail::model_json(r);
    detail::model_put(out, "key", c.query_key);
    detail::model_put(out, "cursor", c.cursor_id);
    detail::model_put(out, "entity", c.entity);
    detail::model_put(out, "direction", unsigned(c.direction));
    detail::model_put(out, "boundary", detail::debug_value(r, c.boundary));
    detail::model_put(out, "page_size", c.page_size);
    detail::model_put(out, "offset", c.next_offset);
    detail::model_put(out, "expires", c.expires_at.unix_seconds());
    detail::model_put(out, "nanos", c.expires_at.subsec_nanos());
    return out;
}

static RetainedIdSet ids_in(Memory r, const Json& j) {
    RetainedIdSet ids(std::allocator_arg, Allocator(r));
    ids.query_key = text(field(j, "key"));
    for (const auto& n : items(j, "ids"))
        ids.ids.push_back(number(n));
    ids.expires_at = expiry(j);
    return ids;
}

static Json ids_out(Memory r, const RetainedIdSet& ids) {
    auto out = detail::model_json(r);
    detail::model_put(out, "key", ids.query_key);
    Json::Array values(r);
    for (auto n : ids.ids)
        values.emplace_back(n);
    detail::model_put(out, "ids", values);
    detail::model_put(out, "expires", ids.expires_at.unix_seconds());
    detail::model_put(out, "nanos", ids.expires_at.subsec_nanos());
    return out;
}

static Json observations(UserContext& c) {
    auto r = c.memory_resource();
    auto out = detail::model_json(r);
    detail::model_put(out, "continuous", *c.continuous_page_plan());
    auto id = c.continuous_page_cursor_id();
    detail::model_put(out, "cursor", id ? Json(r, *id) : Json(r));
    detail::model_put(out, "id_plan", *c.id_set_plan());
    detail::model_put(out, "count", c.id_set_count() ? Json(r, *c.id_set_count()) : Json(r));
    return out;
}

static Json evaluate(UserContext& c, const Json& j) {
    auto r = c.memory_resource();
    auto mode = text(field(j, "mode"));
    auto& cache = c.make<InMemoryAggregationCache>(text(field(j, "namespace")));
    auto& cursors = c.make<InMemoryContinuousPageCursorStore>();
    auto& ids = c.make<InMemoryIdSetStore>();
    detail::QueryStoreAccess::capacity(cursors, number(field(j, "max_entries")));
    detail::QueryStoreAccess::capacity(
        ids, number(field(j, "max_entries")), number(field(j, "max_bytes")));
    Json::Array results(r);
    for (const auto& step : items(j, "steps")) {
        auto op = text(field(step, "op"));
        auto out = detail::model_json(r);
        std::string_view key =
            optional(step, "key") ? text(field(step, "key")) : std::string_view{};
        if (mode == "cache") {
            if (op == "put") {
                auto rows = rows_in(r, field(step, "rows"));
                cache.put(key, rows);
            } else if (op == "get") {
                auto rows = cache.get(r, key, number(field(step, "age")));
                detail::model_put(out, "value", rows ? rows_out(r, *rows) : Json(r));
            } else if (op == "age")
                detail::QueryStoreAccess::age(cache, key, signed_number(field(step, "millis")));
            else if (op == "clear")
                cache.clear();
            else
                cache.invalidate_namespace(key);
            detail::model_put(out, "keys", strings(r, detail::QueryStoreAccess::keys(r, cache)));
            detail::model_put(out, "namespace", cache.name_space());
        } else if (mode == "cursor") {
            if (op == "get") {
                auto& result = cursors.get(r, key, number(field(step, "offset"))).wait();
                if (!result)
                    detail::model_put(out, "error", result.error());
                else
                    detail::model_put(
                        out, "value", result.value() ? cursor_out(r, *result.value()) : Json(r));
            } else {
                auto* work =
                    op == "put" ? &cursors.put(r, cursor_in(r, step)) : &cursors.invalidate(r, key);
                if (auto* cancel = optional(step, "cancel");
                    cancel && std::get<bool>(cancel->data)) {
                    work->cancel();
                    detail::model_put(out, "cancelled", true);
                } else if (auto& result = work->wait(); !result)
                    detail::model_put(out, "error", result.error());
            }
            detail::model_put(out, "keys", strings(r, detail::QueryStoreAccess::keys(r, cursors)));
        } else if (mode == "ids") {
            if (op == "get") {
                auto& result = ids.get(r, key).wait();
                if (!result)
                    detail::model_put(out, "error", result.error());
                else
                    detail::model_put(
                        out, "value", result.value() ? ids_out(r, *result.value()) : Json(r));
            } else {
                auto* work = op == "put" ? &ids.put(r, ids_in(r, step)) : &ids.invalidate(r, key);
                if (auto* cancel = optional(step, "cancel");
                    cancel && std::get<bool>(cancel->data)) {
                    work->cancel();
                    detail::model_put(out, "cancelled", true);
                } else if (auto& result = work->wait(); !result)
                    detail::model_put(out, "error", result.error());
            }
            detail::model_put(out, "keys", strings(r, detail::QueryStoreAccess::keys(r, ids)));
            detail::model_put(out, "bytes", detail::QueryStoreAccess::bytes(ids));
        } else {
            if (op == "continuous") {
                auto* id = optional(step, "cursor");
                detail::ContextPaginationAccess::observe_continuous_page(
                    c,
                    key,
                    id && !std::holds_alternative<std::monostate>(id->data)
                        ? std::optional<std::string_view>(text(*id))
                        : std::nullopt);
            } else if (op == "ids") {
                auto* count = optional(step, "count");
                detail::ContextPaginationAccess::observe_id_set(
                    c,
                    key,
                    count && !std::holds_alternative<std::monostate>(count->data)
                        ? std::optional<std::uint64_t>(number(*count))
                        : std::nullopt);
            }
            out = observations(c);
        }
        results.emplace_back(std::move(out));
    }
    return Json(r, results);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext g;
        auto c = g.make_user_context();
        auto j = Json::parse(c->memory_resource(), line);
        std::cout << evaluate(*c, *j).dump(c->memory_resource()) << '\n';
    }
}
