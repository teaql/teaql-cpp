// Pinned root SQL tests with a synthetic numbered SQLite compiler dialect.
#include <teaql/runtime/context.hpp>
#include <teaql/sql.hpp>
#include <array>
#include <charconv>
#include <iostream>

using namespace teaql;

static_assert(
    std::same_as<sql::SqlBindings, SqlBindings> &&
    std::same_as<sql::SchemaIndexSpec, SchemaIndexSpec> &&
    std::same_as<sql::SqlDialect, SqlDialect> && std::same_as<sql::CompiledQuery, CompiledQuery> &&
    std::same_as<sql::DatabaseKind, DatabaseKind> &&
    std::same_as<sql::SqlCompileError, SqlCompileError> &&
    std::same_as<sql::SqlDataServiceExecutor<String>, SqlDataServiceExecutor<String>> &&
    std::same_as<sql::SqlDataServiceTransaction<String>, SqlDataServiceTransaction<String>> &&
    std::same_as<sql::SqlExecutorError<String>, SqlExecutorError<String>> &&
    std::same_as<sql::SqlTransaction<String>, SqlTransaction<String>> &&
    std::same_as<sql::SqlTransactionTransport<String>, SqlTransactionTransport<String>> &&
    std::same_as<sql::SqlTransport<String>, SqlTransport<String>> &&
    std::same_as<sql::StreamingSqlTransport<String>, StreamingSqlTransport<String>>);
static_assert(&sql::bounded_sql_identifier == &bounded_sql_identifier &&
              &sql::schema_foreign_key_name == &schema_foreign_key_name &&
              &sql::schema_index_specs == &schema_index_specs &&
              &sql::storage_length_covers == &storage_length_covers &&
              &sql::storage_numeric_covers == &storage_numeric_covers &&
              &sql::validate_schema_identifier_lengths == &validate_schema_identifier_lengths &&
              &sql::render_sql_value == &render_sql_value &&
              &sql::render_sql_with == &render_sql_with);
using QuoteBytes = String (*)(Memory, std::string_view, char);
using QuoteScalar = String (*)(Memory, std::string_view, char32_t);
static_assert(static_cast<QuoteBytes>(&sql::quote_identifier_if_needed) ==
                  static_cast<QuoteBytes>(&quote_identifier_if_needed) &&
              static_cast<QuoteScalar>(&sql::quote_identifier_if_needed) ==
                  static_cast<QuoteScalar>(&quote_identifier_if_needed));

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class TestDialect final : public SqlDialect {
public:
    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory owner, std::string_view name) const override {
        String result("\"", owner);
        result += name;
        result += '"';
        return result;
    }

    String placeholder(Memory owner, std::size_t index) const override {
        char digits[32];
        auto end = std::to_chars(digits, digits + sizeof(digits), index).ptr;
        String result("?", owner);
        result.append(digits, end);
        return result;
    }

    // Preserve the original dummy override as a generic hook; no native GBK backend.
    String compile_gbk_function(Memory owner,
                                const EntityDescriptor& entity,
                                std::span<const Expr> args,
                                SqlBindings& bindings) const override {
        if (args.size() != 1) {
            throw SqlCompileError(std::allocator_arg,
                                  Allocator(owner),
                                  SqlCompileError::Code::InvalidFunctionArguments,
                                  "GBK expects exactly one argument");
        }
        String result("convert_to(", owner);
        result += compile_expr(owner, entity, args[0], bindings);
        result += ", 'GBK')";
        return result;
    }
};

static EntityDescriptor entity(Memory owner, bool tenant = false) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.table_name = "orders";
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    result.property("name", DataType::Text);
    if (tenant) {
        result.property("tenant_id", DataType::U64).not_null();
    }
    result.mask_fields({});
    return result;
}

static EntityDescriptor line_entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "OrderLine");
    result.table_name = "orderline";
    result.property("id", DataType::U64).id().not_null();
    result.property("order_id", DataType::U64);
    result.property("name", DataType::Text);
    return result;
}

// Only the synthetic source dialect's binding sigil changes; expected SQL is literal.
static void sql_is(const CompiledQuery& compiled, std::string_view original) {
    String expected(original, compiled.memory_resource());
    for (std::size_t index = 0; index + 1 < expected.size(); ++index) {
        if (expected[index] == '$' && expected[index + 1] >= '0' && expected[index + 1] <= '9') {
            expected[index] = '?';
        }
    }
    check(compiled.sql == expected,
          "complete original compiler SQL with numbered SQLite sigil adaptation");
}

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory owner = user->memory_resource();
    TestDialect dialect;
    EntityDescriptor order = entity(owner);
    EntityDescriptor line = line_entity(owner);

    SelectQuery query() {
        return SelectQuery(std::allocator_arg, Allocator(owner), "Order");
    }

    CompiledQuery compile(const SelectQuery& query) {
        return dialect.compile_select(owner, order, query);
    }
};

static void quotes_identifiers_only_when_needed(Memory owner) {
    check(quote_identifier_if_needed(owner, "stock_item_data", '"') == "stock_item_data" &&
              quote_identifier_if_needed(owner, "select", '"') == "\"select\"" &&
              quote_identifier_if_needed(owner, "order", '`') == "`order`" &&
              quote_identifier_if_needed(owner, "has space", '"') == "\"has space\"" &&
              quote_identifier_if_needed(owner, "\"already_wrapped\"", '"') ==
                  "\"already_wrapped\"",
          "all original generic identifier quoting inputs");
}

static void bounded_identifier_preserves_short_names_and_disambiguates_long_names(Memory owner) {
    check(bounded_sql_identifier(owner, "IDX_ORDER_ID", 63) == "IDX_ORDER_ID",
          "original short index name");
    String left("PK_", owner), right("PK_", owner);
    left.append(56, 'A');
    right.append(55, 'A');
    right += 'B';
    left += "_ID_VERSION";
    right += "_ID_VERSION";
    auto bounded_left = bounded_sql_identifier(owner, left, 63);
    auto bounded_right = bounded_sql_identifier(owner, right, 63);
    check(bounded_left.size() <= 63 && bounded_right.size() <= 63 &&
              bounded_left != bounded_right &&
              bounded_left == bounded_sql_identifier(owner, left, 63),
          "original stable disambiguated long names");
    String unicode("IDX_", owner);
    for (unsigned index = 0; index < 20; ++index) {
        unicode += "学校";
    }
    auto bounded = bounded_sql_identifier(owner, unicode, 64);
    auto last = bounded.back();
    check(bounded.size() <= 64 && ((last >= '0' && last <= '9') || (last >= 'a' && last <= 'f') ||
                                   (last >= 'A' && last <= 'F')),
          "original Unicode bound and ASCII hex suffix");
}

static void generated_index_specs_are_the_source_of_ddl_order_and_columns(Fixture& f) {
    EntityDescriptor descriptor(std::allocator_arg, Allocator(f.owner), "Order");
    descriptor.table_name = "orders";
    descriptor.property("id", DataType::U64).id().not_null();
    descriptor.property("version", DataType::I64).version().not_null();
    descriptor.property("tenantId", DataType::U64).column_name = "tenant_id";
    descriptor.property("create_time", DataType::Timestamp);
    auto specs = schema_index_specs(f.owner, descriptor, {});
    check(specs.size() == 3 && specs[0].name == "PK_ORDERS_ID_VERSION" && specs[0].unique &&
              specs[0].columns ==
                  Vector<String>({String("id", f.owner), String("version", f.owner)}, f.owner) &&
              specs[1].name == "IDX_ORDERS_TENANT_ID" && !specs[1].unique &&
              specs[1].columns == Vector<String>({String("tenant_id", f.owner)}, f.owner) &&
              specs[2].name == "IDX_ORDERS_CREATE_TIME" &&
              specs[2].columns == Vector<String>({String("create_time", f.owner)}, f.owner),
          "all original index specs, order, columns and uniqueness");
    auto sql = f.dialect.schema_indexes_sqls(f.owner, descriptor);
    check(
        sql ==
            Vector<String>(
                {String(
                     R"(CREATE UNIQUE INDEX IF NOT EXISTS "PK_ORDERS_ID_VERSION" ON "orders" ("id", "version"))",
                     f.owner),
                 String(
                     R"(CREATE INDEX IF NOT EXISTS "IDX_ORDERS_TENANT_ID" ON "orders" ("tenant_id"))",
                     f.owner),
                 String(
                     R"(CREATE INDEX IF NOT EXISTS "IDX_ORDERS_CREATE_TIME" ON "orders" ("create_time"))",
                     f.owner)},
                f.owner),
        "complete original three DDL strings");
    check(schema_index_specs(f.owner, descriptor, 63) == specs,
          "original bound does not alter these specs");
}

static void validate_identifier(Memory owner, const EntityDescriptor& entity, std::size_t maximum) {
    const std::array<const EntityDescriptor*, 1> descriptors{&entity};
    validate_schema_identifier_lengths(owner, descriptors, maximum);
}

static SqlCompileError
identifier_error(Memory owner, const EntityDescriptor& entity, std::size_t maximum) {
    try {
        validate_identifier(owner, entity, maximum);
    } catch (const SqlCompileError& error) {
        return detail::clone_in(owner, error);
    }
    throw std::runtime_error("original overlength identifier unexpectedly accepted");
}

static void
schema_identifier_limits_reject_model_tables_and_columns_without_truncation(Memory owner) {
    EntityDescriptor max_table(std::allocator_arg, Allocator(owner), "School");
    max_table.table_name.assign(63, 'a');
    max_table.property("id", DataType::U64).id();
    validate_identifier(owner, max_table, 63);
    auto long_table = detail::clone_in(owner, max_table);
    long_table.table_name += 'a';
    auto error = identifier_error(owner, long_table, 63);
    check(error.code == SqlCompileError::Code::SchemaIdentifierTooLong &&
              error.actual_bytes == 64 && error.max_bytes == 63 &&
              error.to_string(owner).find("table for entity School") != String::npos,
          "original table identifier error payload and context");
    validate_identifier(owner, long_table, 64);
    EntityDescriptor long_column(std::allocator_arg, Allocator(owner), "School");
    long_column.table_name = "school_data";
    auto& phone = long_column.property("contactPhone", DataType::Text);
    phone.column_name.clear();
    for (unsigned index = 0; index < 22; ++index) {
        phone.column_name += "学校";
    }
    auto column_error = identifier_error(owner, long_column, 64).to_string(owner);
    check(column_error.find("column for entity School property contactPhone") != String::npos &&
              column_error.find("132 bytes") != String::npos,
          "original Unicode column error context and byte count");
    EntityDescriptor quoted(std::allocator_arg, Allocator(owner), "School");
    quoted.table_name = "\"";
    quoted.table_name.append(63, 'a');
    quoted.table_name += '"';
    auto& id = quoted.property("id", DataType::U64);
    id.column_name = "`";
    id.column_name.append(64, 'b');
    id.column_name += '`';
    validate_identifier(owner, quoted, 64);
    (void)identifier_error(owner, quoted, 63);
}

static void foreign_key_names_share_stable_provider_bounds(Memory owner) {
    check(schema_foreign_key_name(owner, "school", "platform_id", "platform", "id", 63) ==
              "FK_SCHOOL_PLATFORM_ID_PLATFORM_ID",
          "original short foreign key name");
    String table(owner), column(owner);
    for (unsigned index = 0; index < 3; ++index) {
        table += "school_type_relation_";
    }
    auto bound63 = schema_foreign_key_name(owner, table, "school_type_id", "school_type", "id", 63);
    auto bound64 = schema_foreign_key_name(owner, table, "school_type_id", "school_type", "id", 64);
    check(bound63.size() <= 63 && bound64.size() <= 64 && bound63 != bound64 &&
              bound63 ==
                  schema_foreign_key_name(owner, table, "school_type_id", "school_type", "id", 63),
          "original stable provider-independent explicit bounds");
    for (unsigned index = 0; index < 20; ++index) {
        column += "学校";
    }
    auto unicode = schema_foreign_key_name(owner, "学校", column, "平台", "id", 63);
    check(unicode.size() <= 63 && detail::identity_utf8(unicode),
          "original complete Unicode name boundary");
}

static void compiles_select_with_filters_order_and_limit(Fixture& f) {
    auto query = f.query();
    query.project("id")
        .project("name")
        .filter(Expr::eq(f.owner, "name", "A"))
        .order_desc("id")
        .limit(10)
        .offset(5);
    auto actual = f.compile(query);
    CompiledQuery expected(std::allocator_arg, Allocator(f.owner));
    expected.sql =
        R"(SELECT "id", "name" FROM "orders" WHERE ("name" = ?1) ORDER BY "id" DESC LIMIT 10 OFFSET 5)";
    expected.params.emplace_back("A");
    expected.log_context.database_kind.emplace("sqlite", f.owner);
    expected.log_context.parameter_policies.push_back(SqlParameterLogPolicy::Plain);
    expected.log_context.generated_sql = true;
    check(actual == expected,
          "complete original CompiledQuery including provenance with SQLite kind/sigil adaptation");
}

static void compiles_partitioned_relation_limit_per_parent(Fixture& f) {
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "OrderLine");
    Vector<Value> values(f.owner);
    values.emplace_back(std::uint64_t(11));
    values.emplace_back(std::uint64_t(12));
    query.project("id")
        .project("order_id")
        .project("name")
        .filter(Expr::in_list(f.owner, "order_id", values))
        .order_desc("id")
        .page(1, 3)
        .partition_by("order_id");
    auto compiled = f.dialect.compile_select(f.owner, f.line, query);
    sql_is(
        compiled,
        R"(SELECT * FROM (SELECT "id", "order_id", "name", ROW_NUMBER() OVER (PARTITION BY "order_id" ORDER BY "id" DESC) AS "__teaql_partition_rank" FROM "orderline" WHERE ("order_id" IN ($1, $2))) AS "__teaql_partitioned" WHERE "__teaql_partition_rank" > 1 AND "__teaql_partition_rank" <= 4 ORDER BY "__teaql_partition_rank")");
    check(compiled.params == values, "original per-parent window bindings");
}

static void partitioned_grouping_preserves_having_before_window_slice(Fixture& f) {
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "OrderLine");
    query.group_by("order_id")
        .group_by("name")
        .count("n")
        .filter(Expr::eq(f.owner, "name", "retained"))
        .having(Expr::binary(
            Expr::count_all(f.owner), BinaryOp::Gt, Expr::value(f.owner, std::int64_t(1))))
        .order_asc("name")
        .page(1, 3)
        .partition_by("order_id");
    auto compiled = f.dialect.compile_select(f.owner, f.line, query);
    sql_is(
        compiled,
        R"(SELECT * FROM (SELECT "order_id", "name", COUNT(*) AS "n", ROW_NUMBER() OVER (PARTITION BY "order_id" ORDER BY "name" ASC) AS "__teaql_partition_rank" FROM "orderline" WHERE ("name" = $1) GROUP BY "order_id", "name" HAVING (COUNT(*) > $2)) AS "__teaql_partitioned" WHERE "__teaql_partition_rank" > 1 AND "__teaql_partition_rank" <= 4 ORDER BY "__teaql_partition_rank")");
    Vector<Value> expected(f.owner);
    expected.emplace_back(Value(f.owner, "retained"));
    expected.emplace_back(std::int64_t(1));
    check(compiled.params == expected, "original grouped partition retains WHERE/HAVING bindings");
}

static void deduplicates_partition_projection_for_mysql_derived_tables(Fixture& f) {
    // The source itself uses the generic TestDialect here, not a MySQL backend.
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "OrderLine");
    query.project("id")
        .project("order_id")
        .project("id")
        .order_asc("id")
        .limit(3)
        .partition_by("order_id");
    auto compiled = f.dialect.compile_select(f.owner, f.line, query);
    std::size_t count = 0, position = 0;
    while ((position = compiled.sql.find("\"id\"", position)) != String::npos) {
        ++count;
        position += 4;
    }
    check(count == 2 && compiled.sql.find("\"id\", \"order_id\", \"id\"") == String::npos,
          "original generic derived projection deduplication");
}

static void compiles_aggregate_projection(Fixture& f, bool ordinary_fields) {
    auto query = f.query();
    if (ordinary_fields) {
        query.project("id").project("name");
    }
    query.count_field("id", "count");
    sql_is(f.compile(query), R"(SELECT COUNT("id") AS "count" FROM "orders")");
}

static void compiles_grouped_aggregate_and_extended_predicates(Fixture& f) {
    Vector<Value> names(f.owner);
    names.emplace_back("x");
    names.emplace_back("y");
    auto predicate =
        Expr::between(
            f.owner, "version", Value(f.owner, std::int64_t(1)), Value(f.owner, std::int64_t(9)))
            .and_expr(Expr::not_like(f.owner, "name", "tmp%"))
            .and_expr(Expr::not_in_list(f.owner, "name", names))
            .and_expr(Expr::is_not_null(f.owner, "name"));
    auto query = f.query();
    query.group_by("name")
        .count("total")
        .sum("version", "versionSum")
        .filter(predicate)
        .order_asc("name");
    auto compiled = f.compile(query);
    sql_is(
        compiled,
        R"(SELECT "name", COUNT(*) AS "total", SUM("version") AS "versionSum" FROM "orders" WHERE (("version" BETWEEN $1 AND $2) AND ("name" NOT LIKE $3) AND ("name" NOT IN ($4, $5)) AND ("name" IS NOT NULL)) GROUP BY "name" ORDER BY "name" ASC)");
    Vector<Value> expected(f.owner);
    expected.emplace_back(std::int64_t(1));
    expected.emplace_back(std::int64_t(9));
    expected.emplace_back("tmp%");
    expected.emplace_back("x");
    expected.emplace_back("y");
    check(compiled.params == expected,
          "original complete grouped predicate binding order and variants");
}

static void compiles_sound_like_expression(Fixture& f) {
    auto query = f.query();
    query.filter(Expr::sound_like(f.owner, "name", "Robert"));
    auto actual = f.compile(query);
    CompiledQuery expected(std::allocator_arg, Allocator(f.owner));
    expected.sql =
        R"(SELECT "id", "version", "name" FROM "orders" WHERE (SOUNDEX("name") = SOUNDEX(?1)))";
    expected.params.emplace_back("Robert");
    expected.log_context.database_kind.emplace("sqlite", f.owner);
    expected.log_context.parameter_policies.push_back(SqlParameterLogPolicy::Plain);
    expected.log_context.generated_sql = true;
    check(actual == expected,
          "complete original sound-like CompiledQuery with source-dummy adaptation");
}

static void compiles_java_style_string_match_builders(Fixture& f) {
    auto predicate = Expr::contain(f.owner, "name", "tea")
                         .and_expr(Expr::begin_with(f.owner, "name", "t"))
                         .and_expr(Expr::end_with(f.owner, "name", "a"))
                         .and_expr(Expr::not_contain(f.owner, "name", "coffee"))
                         .and_expr(Expr::not_begin_with(f.owner, "name", "x"))
                         .and_expr(Expr::not_end_with(f.owner, "name", "z"));
    auto query = f.query();
    query.filter(predicate);
    auto compiled = f.compile(query);
    sql_is(
        compiled,
        R"(SELECT "id", "version", "name" FROM "orders" WHERE (("name" LIKE $1) AND ("name" LIKE $2) AND ("name" LIKE $3) AND ("name" NOT LIKE $4) AND ("name" NOT LIKE $5) AND ("name" NOT LIKE $6)))");
    Vector<Value> expected(f.owner);
    for (auto value : {"%tea%", "t%", "%a", "%coffee%", "x%", "%z"}) {
        expected.emplace_back(value);
    }
    check(compiled.params == expected, "all six original string-match patterns in order");
}

static void compiles_search_with_text(Fixture& f) {
    auto query = f.query();
    query.search_with_text("AI");
    auto compiled = f.compile(query);
    sql_is(compiled, R"(SELECT "id", "version", "name" FROM "orders" WHERE ("name" LIKE $1))");
    check(compiled.params.size() == 1 && compiled.params[0] == Value(f.owner, "%AI%"),
          "original search binding");
}

static void compiles_insert_update_delete_and_recover(Fixture& f) {
    InsertCommand insert(std::allocator_arg, Allocator(f.owner), "Order");
    insert.value("id", std::uint64_t(1)).value("name", "A");
    sql_is(f.dialect.compile_insert(f.owner, f.order, insert),
           R"(INSERT INTO "orders" ("id", "name") VALUES ($1, $2))");
    UpdateCommand update(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1));
    update.expected_version(3).value("name", "B");
    sql_is(f.dialect.compile_update(f.owner, f.order, update),
           R"(UPDATE "orders" SET "name" = $1, "version" = $2 WHERE "id" = $3 AND "version" = $4)");
    DeleteCommand deletion(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1));
    deletion.expected_version(3);
    sql_is(f.dialect.compile_delete(f.owner, f.order, deletion),
           R"(UPDATE "orders" SET "version" = $1 WHERE "id" = $2 AND "version" = $3)");
    RecoverCommand recovery(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1), -4);
    sql_is(f.dialect.compile_recover(f.owner, f.order, recovery),
           R"(UPDATE "orders" SET "version" = $1 WHERE "id" = $2 AND "version" = $3)");
}

static void guarded_mutations_compile_tenant_predicate_in_the_same_statement(Fixture& f) {
    auto tenant = entity(f.owner, true);
    auto guard = Expr::eq(f.owner, "tenant_id", std::uint64_t(7));
    UpdateCommand update(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1));
    update.expected_version(3).value("name", "B");
    auto updated = f.dialect.compile_guarded_update(f.owner, tenant, update, guard);
    sql_is(
        updated,
        R"(UPDATE "orders" SET "name" = $1, "version" = $2 WHERE "id" = $3 AND "version" = $4 AND ("tenant_id" = $5))");
    Vector<Value> expected(f.owner);
    expected.emplace_back("B");
    expected.emplace_back(std::int64_t(4));
    expected.emplace_back(std::uint64_t(1));
    expected.emplace_back(std::int64_t(3));
    expected.emplace_back(std::uint64_t(7));
    check(updated.params == expected, "original complete guarded-update bindings");
    DeleteCommand deletion(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1));
    deletion.expected_version(3);
    auto deleted = f.dialect.compile_guarded_delete(f.owner, tenant, deletion, guard);
    sql_is(
        deleted,
        R"(UPDATE "orders" SET "version" = $1 WHERE "id" = $2 AND "version" = $3 AND ("tenant_id" = $4))");
    expected.clear();
    expected.emplace_back(std::int64_t(-4));
    expected.emplace_back(std::uint64_t(1));
    expected.emplace_back(std::int64_t(3));
    expected.emplace_back(std::uint64_t(7));
    check(deleted.params == expected, "original complete guarded-soft-delete bindings");
    deletion.hard_delete();
    auto hard = f.dialect.compile_guarded_delete(f.owner, tenant, deletion, guard);
    sql_is(hard,
           R"(DELETE FROM "orders" WHERE "id" = $1 AND "version" = $2 AND ("tenant_id" = $3))");
    expected.clear();
    expected.emplace_back(std::uint64_t(1));
    expected.emplace_back(std::int64_t(3));
    expected.emplace_back(std::uint64_t(7));
    check(hard.params == expected, "original complete guarded-hard-delete bindings");
    RecoverCommand recovery(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1), -4);
    auto recovered = f.dialect.compile_guarded_recover(f.owner, tenant, recovery, guard);
    sql_is(
        recovered,
        R"(UPDATE "orders" SET "version" = $1 WHERE "id" = $2 AND "version" = $3 AND ("tenant_id" = $4))");
    expected.clear();
    expected.emplace_back(std::int64_t(5));
    expected.emplace_back(std::uint64_t(1));
    expected.emplace_back(std::int64_t(-4));
    expected.emplace_back(std::uint64_t(7));
    check(recovered.params == expected, "original complete guarded-recover bindings");
}

static void compiles_in_expression_and_validates_empty_list(Fixture& f) {
    Vector<Value> values(f.owner);
    values.emplace_back(std::uint64_t(1));
    values.emplace_back(std::uint64_t(2));
    auto query = f.query();
    query.filter(
        Expr::binary(Expr::column(f.owner, "id"), BinaryOp::In, Expr::value(f.owner, values)));
    sql_is(f.compile(query),
           R"(SELECT "id", "version", "name" FROM "orders" WHERE ("id" IN ($1, $2)))");
    values.clear();
    query.filter(
        Expr::binary(Expr::column(f.owner, "id"), BinaryOp::In, Expr::value(f.owner, values)));
    bool rejected = false;
    try {
        (void)f.compile(query);
    } catch (const SqlCompileError& error) {
        rejected = error.code == SqlCompileError::Code::EmptyInList;
    }
    check(rejected, "original empty IN list retains its structured error");
}

static void generic_dialect_expands_large_in_expressions(Fixture& f) {
    Vector<Value> ids(f.owner), names(f.owner);
    ids.emplace_back(std::uint64_t(1));
    ids.emplace_back(std::uint64_t(2));
    names.emplace_back("archived");
    auto query = f.query();
    query.filter(
        Expr::in_large(f.owner, "id", ids).and_expr(Expr::not_in_large(f.owner, "name", names)));
    auto compiled = f.compile(query);
    sql_is(
        compiled,
        R"(SELECT "id", "version", "name" FROM "orders" WHERE (("id" IN ($1, $2)) AND ("name" NOT IN ($3))))");
    ids.emplace_back("archived");
    check(compiled.params == ids,
          "original explicit-large operators expand exact typed list bindings");
}

static void compiles_property_to_property_filters(Fixture& f) {
    auto query = f.query();
    query.filter(Expr::compare_columns(f.owner, "version", BinaryOp::Gte, "id"));
    auto compiled = f.compile(query);
    sql_is(compiled, R"(SELECT "id", "version", "name" FROM "orders" WHERE ("version" >= "id"))");
    check(compiled.params.empty(), "original property comparison has no value bindings");
}

static void compiles_raw_escape_hatches_and_dynamic_properties(Fixture& f) {
    auto query = f.query();
    query.comment("audit")
        .project("id")
        .project_raw("name", "upper(name)")
        .dynamic_property_raw("score", "42")
        .add_raw_sql_search_criteria("name <> ''")
        .add_raw_sql_search_criteria(R"(payload @> '{"active":true}')");
    auto compiled = f.compile(query);
    constexpr std::string_view expected =
        R"(SELECT "id", upper(name) AS "name", 42 AS "score" FROM "orders" WHERE name <> '' AND payload @> '{"active":true}')";
    check(compiled.sql == expected && compiled.comment == "audit" &&
              compiled.sql_with_comment(f.owner) ==
                  String("/* audit */ ", f.owner) + String(expected, f.owner),
          "all original opaque raw fragments, aliases and comment forms");
}

static void compiles_raw_sql_override_with_comment(Fixture& f) {
    auto query = f.query();
    query.comment("manual").raw_sql("SELECT 1 AS id");
    auto compiled = f.compile(query);
    check(compiled.sql == "SELECT 1 AS id" && compiled.comment == "manual" &&
              compiled.sql_with_comment(f.owner) == "/* manual */ SELECT 1 AS id",
          "original raw SQL override/comment");
}

static void compiles_subquery_expression_and_appends_params_in_order(Fixture& f) {
    SelectQuery child(std::allocator_arg, Allocator(f.owner), "OrderLine");
    child.filter(Expr::eq(f.owner, "name", "line-1")).order_asc("id").limit(10);
    auto query = f.query();
    query.filter(Expr::in_subquery(f.owner, "id", f.line, child, "order_id")
                     .and_expr(Expr::eq(f.owner, "name", "order-1")));
    auto compiled = f.compile(query);
    sql_is(
        compiled,
        R"(SELECT "id", "version", "name" FROM "orders" WHERE (("id" IN (SELECT "order_id" FROM "orderline" WHERE ("name" = $1) ORDER BY "id" ASC LIMIT 10)) AND ("name" = $2)))");
    check(compiled.params ==
              Vector<Value>({Value(f.owner, "line-1"), Value(f.owner, "order-1")}, f.owner),
          "original complete subquery/outer binding order");
}

static void
compiles_extended_aggregates_expression_projection_function_order_and_having(Fixture& f) {
    auto query = f.query();
    query.group_by("name")
        .project_expr("nameSound", Expr::soundex(Expr::column(f.owner, "name")))
        .stddev("version", "stddevVersion")
        .stddev_pop("version", "stddevPopVersion")
        .var_samp("version", "varSampVersion")
        .var_pop("version", "varPopVersion")
        .bit_and("version", "bitAndVersion")
        .bit_or("version", "bitOrVersion")
        .bit_xor("version", "bitXorVersion")
        .having(Expr::binary(
            Expr::count_all(f.owner), BinaryOp::Gt, Expr::value(f.owner, std::int64_t(1))))
        .order_gbk_asc("name");
    auto compiled = f.compile(query);
    sql_is(
        compiled,
        R"(SELECT "name", SOUNDEX("name") AS "nameSound", STDDEV("version") AS "stddevVersion", STDDEV_POP("version") AS "stddevPopVersion", VAR_SAMP("version") AS "varSampVersion", VAR_POP("version") AS "varPopVersion", BIT_AND("version") AS "bitAndVersion", BIT_OR("version") AS "bitOrVersion", BIT_XOR("version") AS "bitXorVersion" FROM "orders" GROUP BY "name" HAVING (COUNT(*) > $1) ORDER BY convert_to("name", 'GBK') ASC)");
    check(compiled.params.size() == 1 && compiled.params[0] == Value(f.owner, std::int64_t(1)),
          "original extended aggregate query binding");
}

static void renders_sqlite_debug_sql_with_inlined_params(Memory owner) {
    CompiledQuery query(std::allocator_arg, Allocator(owner));
    query.sql = R"(UPDATE "orders" SET "name" = ? WHERE (("id" = ?) AND ('?' = '?')))";
    query.params.emplace_back("Alice's Shop");
    query.params.emplace_back(std::uint64_t(7));
    check(query.debug_sql(owner, DatabaseKind::Sqlite) ==
              R"(UPDATE "orders" SET "name" = 'Alice''s Shop' WHERE (("id" = 7) AND ('?' = '?')))",
          "original SQLite escaped text and untouched quoted question marks");
}

static void sqlite_debug_sql_preserves_comments_and_temporal_storage_literals(Memory owner) {
    CompiledQuery query(std::allocator_arg, Allocator(owner));
    query.sql = "-- line ? $1\nSELECT '?', \"identifier?\", ?, ? /* block ? */";
    query.params.emplace_back(*Date::parse("2024-02-29"));
    query.params.emplace_back(Timestamp(1787110200123LL));
    query.comment.emplace("teaql purpose=temporal.verify ? $1", owner);
    check(
        query.debug_sql(owner, DatabaseKind::Sqlite) ==
            "/* teaql purpose=temporal.verify ? $1 */ -- line ? $1\nSELECT '?', \"identifier?\", '2024-02-29', 1787110200123 /* block ? */",
        "original SQLite complete comment/date/millisecond diagnostic rendering");
}

static void unsupported_backend_boundaries(Memory owner) {
    for (auto kind : {DatabaseKind::PostgreSql, DatabaseKind::MySql}) {
        auto result = render_sql_value(owner, Value(owner, std::int64_t(1)), kind);
        check(!result, "out-of-scope native rendering rejects unsupported backend kind");
    }
}

static void original_contracts(Fixture& f) {
    quotes_identifiers_only_when_needed(f.owner);
    bounded_identifier_preserves_short_names_and_disambiguates_long_names(f.owner);
    generated_index_specs_are_the_source_of_ddl_order_and_columns(f);
    schema_identifier_limits_reject_model_tables_and_columns_without_truncation(f.owner);
    foreign_key_names_share_stable_provider_bounds(f.owner);
    compiles_select_with_filters_order_and_limit(f);
    compiles_partitioned_relation_limit_per_parent(f);
    partitioned_grouping_preserves_having_before_window_slice(f);
    deduplicates_partition_projection_for_mysql_derived_tables(f);
    compiles_aggregate_projection(f, false);
    compiles_aggregate_projection(f, true);
    compiles_grouped_aggregate_and_extended_predicates(f);
    compiles_sound_like_expression(f);
    compiles_java_style_string_match_builders(f);
    compiles_search_with_text(f);
    check(f.dialect.schema_setup_sqls().empty(), "original default schema setup is empty");
    compiles_insert_update_delete_and_recover(f);
    guarded_mutations_compile_tenant_predicate_in_the_same_statement(f);
    compiles_in_expression_and_validates_empty_list(f);
    generic_dialect_expands_large_in_expressions(f);
    compiles_property_to_property_filters(f);
    compiles_raw_escape_hatches_and_dynamic_properties(f);
    compiles_raw_sql_override_with_comment(f);
    compiles_subquery_expression_and_appends_params_in_order(f);
    compiles_extended_aggregates_expression_projection_function_order_and_having(f);
    renders_sqlite_debug_sql_with_inlined_params(f.owner);
    sqlite_debug_sql_preserves_comments_and_temporal_storage_literals(f.owner);
    unsupported_backend_boundaries(f.owner);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        Fixture f;
        // Catch scoped errors while their allocator and diagnostic fields remain alive.
        try {
            original_contracts(f);
        } catch (const SqlCompileError& error) {
            std::cerr << "Unexpected SQL compile error code " << static_cast<unsigned>(error.code)
                      << '\n';
            std::pmr::set_default_resource(previous);
            return 1;
        }
        std::pmr::set_default_resource(previous);
        std::cout << "Original twenty-six applicable SQL compiler contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
