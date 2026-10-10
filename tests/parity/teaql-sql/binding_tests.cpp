// Semantic port of all ten locked SQL binding-policy test contracts.
#include <teaql/sql/dialect.hpp>
#include <teaql/sql/bindings.hpp>
#include <teaql/memory/arena.hpp>
#include <algorithm>
#include <atomic>
#include <thread>

using namespace teaql;
using Policy = SqlParameterLogPolicy;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Dialect final : SqlDialect {
    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory r, std::string_view name) const override {
        String result("\"", r);
        result += name;
        result += '"';
        return result;
    }

    String placeholder(Memory r, std::size_t) const override {
        return String("?", r);
    }
};

static EntityDescriptor entity(Memory r) {
    EntityDescriptor result(std::allocator_arg, Allocator(r), "Customer");
    result.property("id", DataType::U64).id();
    result.property("version", DataType::I64).version();
    result.property("display_name", DataType::Text).column("customer_name");
    result.property("status", DataType::Text);
    result.property("password", DataType::Text);
    result.mask_fields({"display_name"});
    return result;
}

static Expr all(Memory r, Vector<Expr> clauses) {
    Expr result(r, Expr::Kind::And);
    result.args.swap(clauses);
    return result;
}

static void policies(const CompiledQuery& query, std::initializer_list<Policy> expected) {
    check(query.params.size() == query.log_context.parameter_policies.size() &&
              std::ranges::equal(query.log_context.parameter_policies, expected),
          "binding policies and parameter count match the original contract");
}

static void descriptor_declaration(Memory r) {
    Dialect dialect;
    EntityDescriptor legacy(std::allocator_arg, Allocator(r), "Customer");
    legacy.property("display_name", DataType::Text);
    legacy.property("password", DataType::Text);
    SelectQuery query(std::allocator_arg, Allocator(r), "Customer");
    Vector<Expr> clauses(r);
    clauses.emplace_back(Expr::eq(r, "display_name", "CUSTOMER-CANARY"));
    clauses.emplace_back(Expr::eq(r, "password", "PASSWORD-CANARY"));
    query.filter(all(r, std::move(clauses)));
    auto compiled = dialect.compile_select(r, legacy, query);
    policies(compiled, {Policy::Unknown, Policy::Credential});
    check(compiled.params[0].try_text() == "CUSTOMER-CANARY",
          "privacy classification does not alter values");
    legacy.mask_fields({});
    policies(dialect.compile_select(r, legacy, query), {Policy::Plain, Policy::Credential});
}

static void predicates_and_scalar_scope(Memory r) {
    Dialect dialect;
    auto root = entity(r);
    SelectQuery query(std::allocator_arg, Allocator(r), "Customer");
    Vector<Expr> clauses(r);
    clauses.emplace_back(Expr::eq(r, "display_name", "Riverside"));
    clauses.emplace_back(Expr::eq(r, "status", "ACTIVE"));
    clauses.emplace_back(Expr::eq(r, "password", "CREDENTIAL-CANARY"));
    clauses.emplace_back(Expr::eq(r, "id", std::uint64_t(1)));
    query.filter(all(r, std::move(clauses)));
    auto compiled = dialect.compile_select(r, root, query);
    policies(compiled, {Policy::Masked, Policy::Plain, Policy::Credential, Policy::Plain});
    check(compiled.log_context.generated_sql, "ordinary compiler SQL has trusted provenance");
    Vector<Expr> scalar(r);
    scalar.emplace_back(Expr::binary(Expr::soundex(Expr::column(r, "display_name")),
                                     BinaryOp::Eq,
                                     Expr::soundex(Expr::value(r, "Riverside"))));
    scalar.emplace_back(
        Expr::between(r, "id", Value(r, std::uint64_t(1)), Value(r, std::uint64_t(10))));
    Vector<Value> names(r);
    names.emplace_back("Alpha");
    names.emplace_back("Beta");
    scalar.emplace_back(Expr::in_list(r, "display_name", names));
    scalar.emplace_back(Expr::eq(r, "status", "ACTIVE"));
    query.filter(all(r, std::move(scalar)));
    policies(dialect.compile_select(r, root, query),
             {Policy::Masked,
              Policy::Plain,
              Policy::Plain,
              Policy::Masked,
              Policy::Masked,
              Policy::Plain});
}

static void subquery_and_search_scope(Memory r) {
    Dialect dialect;
    auto root = entity(r);
    EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
    child.property("id", DataType::U64).id();
    child.property("display_name", DataType::Text);
    SelectQuery nested(std::allocator_arg, Allocator(r), "Child");
    nested.filter(Expr::eq(r, "display_name", "public-child"));
    SelectQuery query(std::allocator_arg, Allocator(r), "Customer");
    Vector<Expr> clauses(r);
    clauses.emplace_back(Expr::in_subquery(r, "display_name", child, nested, "display_name"));
    clauses.emplace_back(Expr::eq(r, "display_name", "private-parent"));
    query.filter(all(r, std::move(clauses)));
    policies(dialect.compile_select(r, root, query), {Policy::Unknown, Policy::Masked});
    SelectQuery search(std::allocator_arg, Allocator(r), "Customer");
    search.search_text.emplace("needle", r);
    policies(dialect.compile_select(r, root, search),
             {Policy::Masked, Policy::Plain, Policy::Credential});
    SelectQuery projected(std::allocator_arg, Allocator(r), "Customer");
    projected.project_expr("literal", Expr::value(r, "unclassified"));
    policies(dialect.compile_select(r, root, projected), {Policy::Unknown});
}

static void raw_fragments_taint_the_statement(Memory r) {
    Dialect dialect;
    auto root = entity(r);
    for (unsigned variant = 0; variant < 5; ++variant) {
        SelectQuery query(std::allocator_arg, Allocator(r), "Customer");
        if (variant == 0) {
            query.raw_sql("SELECT 'RAW-CANARY'");
        } else if (variant == 1) {
            query.project_raw("display_name", "'RAW-CANARY'");
        } else if (variant == 2) {
            query.dynamic_property_raw("custom", "'RAW-CANARY'");
        } else if (variant == 3) {
            query.raw_sql_search_criteria.emplace_back("status = 'RAW-CANARY'");
        } else {
            SelectQuery nested(std::allocator_arg, Allocator(r), "Customer");
            nested.raw_sql_search_criteria.emplace_back("status = 'RAW-CANARY'");
            query.filter(Expr::in_subquery(r, "id", root, nested, "id"));
        }
        check(!dialect.compile_select(r, root, query).log_context.generated_sql,
              "raw SQL taints provenance across projection, criteria and subquery boundaries");
    }
}

static void mutation_and_batch_policy(Memory r) {
    Dialect dialect;
    auto root = entity(r);
    InsertCommand insert(std::allocator_arg, Allocator(r), "Customer");
    insert.value("id", std::uint64_t(1))
        .value("display_name", "Riverside")
        .value("status", "ACTIVE")
        .value("password", "secret");
    policies(dialect.compile_insert(r, root, insert),
             {Policy::Plain, Policy::Masked, Policy::Plain, Policy::Credential});
    UpdateCommand update(std::allocator_arg, Allocator(r), "Customer", std::uint64_t(1));
    update.expected_version(2).value("display_name", "Riverside");
    policies(
        dialect.compile_guarded_update(r, root, update, Expr::eq(r, "password", "guard-secret")),
        {Policy::Masked, Policy::Plain, Policy::Plain, Policy::Plain, Policy::Credential});
    DeleteCommand deleted(std::allocator_arg, Allocator(r), "Customer", std::uint64_t(1));
    deleted.expected_version(2);
    policies(
        dialect.compile_guarded_delete(r, root, deleted, Expr::eq(r, "display_name", "Riverside")),
        {Policy::Plain, Policy::Plain, Policy::Plain, Policy::Masked});
    RecoverCommand recovered(std::allocator_arg, Allocator(r), "Customer", std::uint64_t(1), -3);
    policies(dialect.compile_guarded_recover(r, root, recovered, Expr::eq(r, "status", "ACTIVE")),
             {Policy::Plain, Policy::Plain, Policy::Plain, Policy::Plain});
    BatchInsertCommand batch(std::allocator_arg, Allocator(r), "Customer");
    for (std::uint64_t id : {1, 2}) {
        InsertCommand row(std::allocator_arg, Allocator(r), "Customer");
        row.value("id", id).value("display_name", "Riverside").value("password", "secret");
        batch.batch_values.emplace_back(row.values);
    }
    policies(dialect.compile_batch_insert(r, root, batch),
             {Policy::Plain,
              Policy::Masked,
              Policy::Credential,
              Policy::Plain,
              Policy::Masked,
              Policy::Credential});
    BatchUpdateCommand changed(std::allocator_arg, Allocator(r), "Customer", {"display_name"});
    changed.batch_ids.emplace_back(std::uint64_t(1));
    changed.batch_ids.emplace_back(std::uint64_t(2));
    for (const auto& row : batch.batch_values) {
        changed.batch_values.emplace_back(row);
    }
    changed.batch_expected_versions.emplace_back(1);
    changed.batch_expected_versions.emplace_back(2);
    policies(dialect.compile_batch_update(r, root, changed),
             {Policy::Plain,
              Policy::Masked,
              Policy::Plain,
              Policy::Masked,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain,
              Policy::Plain});
}

static void error_and_concurrent_scopes(Memory r) {
    Dialect dialect;
    auto root = entity(r);
    SqlBindings bindings(r);
    bool failed = false;
    try {
        dialect.compile_expr(r, root, Expr::eq(r, "absent", "private"), bindings);
    } catch (const SqlCompileError& error) {
        failed = error.code == SqlCompileError::Code::UnknownField;
    }
    check(failed, "unknown field yields the original structured error");
    dialect.compile_expr(r, root, Expr::eq(r, "status", "ACTIVE"), bindings);
    check(bindings.log_context(r, DatabaseKind::Sqlite).parameter_policies ==
              Vector<Policy>({Policy::Plain}, r),
          "exception restores the active field-policy scope");
    std::atomic<bool> success{true};
    std::vector<std::thread> workers;
    for (unsigned index = 0; index < 64; ++index) {
        workers.emplace_back([&, index] {
            try {
                Arena memory(4096, std::pmr::new_delete_resource());
                auto owner = entity(&memory);
                SelectQuery query(std::allocator_arg, Allocator(&memory), "Customer");
                query.filter(
                    Expr::eq(&memory, index % 2 == 0 ? "display_name" : "status", "shared-value"));
                policies(dialect.compile_select(&memory, owner, query),
                         {index % 2 == 0 ? Policy::Masked : Policy::Plain});
            } catch (...) {
                success.store(false);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    check(success.load(), "64 independent compiler scopes never share binding policies");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    Arena memory(4096, std::pmr::new_delete_resource());
    descriptor_declaration(&memory);
    predicates_and_scalar_scope(&memory);
    subquery_and_search_scope(&memory);
    raw_fragments_taint_the_statement(&memory);
    mutation_and_batch_policy(&memory);
    error_and_concurrent_scopes(&memory);
}
