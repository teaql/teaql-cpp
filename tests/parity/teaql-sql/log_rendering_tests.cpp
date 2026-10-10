// SQLite portions of the six locked shared SQL-renderer test contracts.
// Other database kinds remain explicitly unsupported in the edge migration.
#include <teaql/sql/types.hpp>
#include <teaql/memory/arena.hpp>
#include <limits>

using namespace teaql;
using Db = DatabaseKind;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void projected_values_share_the_debug_renderer(Memory r) {
    for (auto kind : {Db::Sqlite}) {
        CompiledQuery query(std::allocator_arg, Allocator(r));
        query.sql = kind == Db::PostgreSql ? "SELECT $1, $2" : "SELECT ?, ?";
        query.params.emplace_back("O'Reilly");
        query.params.emplace_back(std::int64_t(23));
        check(query.debug_sql(r, kind) == "SELECT 'O''Reilly', 23", "shared debug interpolation");
        auto projection = [&](std::size_t index) {
            if (index == 0) {
                return SqlRenderResult::ok(r, String("'O''****ly' /* masked */", r));
            }
            return render_sql_value(r, query.params[index], kind);
        };
        auto safe = render_sql_with(r, query.sql, kind, 2, projection);
        check(safe && safe.value() == "SELECT 'O''****ly' /* masked */, 23" &&
                  query.params[0].try_text() == "O'Reilly",
              "projection shares the lexer without mutating source bindings");
    }
}

static void quotes_comments_and_repeated_bindings(Memory r) {
    auto constant = [&](std::size_t) {
        return SqlRenderResult::ok(r, String("17", r));
    };
    for (auto kind : {Db::Sqlite}) {
        auto result = render_sql_with(
            r, "SELECT '?', \"?\", `?`, 'O''?雪', ? /* ? */ -- ?\n", kind, 1, constant);
        check(result && result.value() == "SELECT '?', \"?\", `?`, 'O''?雪', 17 /* ? */ -- ?\n",
              "quoted text, comments and Unicode do not consume parameters");
    }
    auto bracket = render_sql_with(r, "SELECT [?], ?", Db::Sqlite, 1, constant);
    check(bracket && bracket.value() == "SELECT [?], 17",
          "SQLite bracket identifiers stay literal");
    auto comments = render_sql_with(r, "SELECT ? /* outer /* ? */ ? */", Db::Sqlite, 1, constant);
    check(comments && comments.value() == "SELECT 17 /* outer /* ? */ ? */",
          "nested comments do not consume SQLite bindings");
}

static void incomplete_or_ambiguous_bindings_fail_closed(Memory r) {
    struct Case {
        const char* sql;
        Db kind;
        std::size_t count;
    };

    for (const auto& input : {Case{"SELECT ?", Db::Sqlite, 0},
                              {"SELECT 1", Db::Sqlite, 1},
                              {"SELECT ?1", Db::Sqlite, 1},
                              {"SELECT :name", Db::Sqlite, 1},
                              {"SELECT @name", Db::Sqlite, 1},
                              {"SELECT $name", Db::Sqlite, 1},
                              {"SELECT 'unterminated", Db::Sqlite, 0},
                              {"SELECT /* unterminated", Db::Sqlite, 0}}) {
        auto safe = [&](std::size_t) {
            return SqlRenderResult::ok(r, String("safe", r));
        };
        check(!render_sql_with(r, input.sql, input.kind, input.count, safe),
              "every original ambiguous or incomplete binding case fails closed");
    }
}

static void projection_failures_and_unsupported_values(Memory r) {
    auto failure = [&](std::size_t) {
        return SqlRenderResult::err(r, "unsupported value");
    };
    auto result = render_sql_with(r, "SELECT ?", Db::Sqlite, 1, failure);
    check(!result && result.error() == "unsupported value",
          "projection errors propagate unchanged");
    CompiledQuery private_query(std::allocator_arg, Allocator(r));
    private_query.sql = "SELECT 'PRIVATE' /*";
    auto text = private_query.debug_sql(r, Db::Sqlite);
    check(text.find("NOT REPLAYABLE") != String::npos && text.find("PRIVATE") == String::npos,
          "failed debug rendering never echoes untrusted original SQL");
    auto null = render_sql_value(r, Value(r), Db::Sqlite);
    check(null && null.value() == "NULL", "null literal rendering");
    for (auto value : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()}) {
        Value::List nested(r);
        nested.emplace_back(value);
        check(!render_sql_value(r, Value(r, value), Db::Sqlite) &&
                  !render_sql_value(r, Value(r, nested), Db::Sqlite),
              "nonfinite scalar and nested numbers are not replayable");
    }
    for (auto unsupported : {Db::MySql, Db::PostgreSql}) {
        auto safe = [&](std::size_t) {
            return SqlRenderResult::ok(r, String("safe", r));
        };
        check(!render_sql_value(r, Value(r, std::int64_t(1)), unsupported) &&
                  !render_sql_with(r, "SELECT ?", unsupported, 1, safe),
              "other database kinds fail closed without adding unsupported backends");
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    Arena memory(4096, std::pmr::new_delete_resource());
    projected_values_share_the_debug_renderer(&memory);
    quotes_comments_and_repeated_bindings(&memory);
    incomplete_or_ambiguous_bindings_fail_closed(&memory);
    projection_failures_and_unsupported_values(&memory);
}
