#include <teaql/core/meta.hpp>
#include <teaql/core/mutation.hpp>
#include <teaql/core/query.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include <limits>
#include <type_traits>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(!std::is_convertible_v<EntitySnapshot, MutationValues>);
static_assert(!std::is_convertible_v<GeneratedValues, MutationValues>);
static_assert(!std::is_default_constructible_v<SelectQuery>);
static_assert(!std::is_copy_constructible_v<Value>);
static_assert(ArenaObject<SelectQuery> && ArenaObject<InsertCommand> &&
              ArenaObject<EntityDescriptor>);

int main() {
    try {
        GlobalContext global;
        auto context = global.make_user_context();
        auto r = context->memory_resource();
        // Source: Rust core/lib.rs default_table_name_matches_java_sql_repository_rule.
        check(default_table_name(r, "OrderLine") == "order_line_data", "table naming");
        check(default_table_name(r, "URL") == "u_r_l_data", "Rust acronym naming");
        auto& entity = global.make<EntityDescriptor>("Order");
        entity.property("id", DataType::U64).id();
        entity.property("version", DataType::I64).version();
        entity.property("name", DataType::Text, "display_name");
        check(entity.property_by_name("name")->column_name == "display_name", "column mapping");
        check(!entity.audit_mask_fields_declared, "5.0.5 unknown provenance by default");
        entity.mask_fields({});
        check(entity.audit_mask_fields_declared, "explicit empty mask declaration");
        check(!Value(r, std::numeric_limits<std::uint64_t>::max()).try_i64(), "u64 overflow");
        check(!Value(r, -1).try_u64(), "negative unsigned conversion");
        check(Value(r, std::uint64_t(INT64_MAX)).try_i64() == INT64_MAX, "i64 boundary");
        check(Value(r, TypedNull{DataType::Text}).is_null(), "typed null");
        check(Value(r, TypedNull{DataType::Text}) != Value(r, nullptr), "null type retained");
        auto filter = Expr::eq(r, "name", "tea");
        check(filter.and_expr(filter) == filter, "deduplication");
        auto joined = filter.and_expr(Expr::gt(r, "version", 0)).and_expr(filter);
        check(joined.args.size() == 2, "joined deduplication");
        Vector<Value> twenty(r);
        for (int i = 0; i < 20; ++i) {
            twenty.emplace_back(i);
        }
        check(Expr::in_list(r, "id", std::move(twenty)).op == BinaryOp::In, "small IN");
        Vector<Value> twenty_one(r);
        for (int i = 0; i < 21; ++i) {
            twenty_one.emplace_back(i);
        }
        check(Expr::in_list(r, "id", std::move(twenty_one)).op == BinaryOp::InLarge, "large IN");
        // Cross-domain clone is explicit, recursive and independent of the source.
        auto copied = joined.clone(global.memory_resource());
        copied.args.front().args.back().literal = Value(global.memory_resource(), "changed");
        check(copied != joined, "AST copy isolation");
        check(copied.args.front().name.get_allocator().resource() == global.memory_resource(),
              "AST clone allocator");
        auto& query = context->make<SelectQuery>("Order");
        check(query.prepare_for_list().slice->limit == 10'000, "default hard limit");
        check(!query.slice, "prepare does not mutate source");
        bool rejected = false;
        try {
            (void)query.limit(10'001).prepare_for_list();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "hard limit violation");
        check(query.hard_limit(20'000).prepare_for_list().slice->limit == 10'001,
              "explicit ceiling");
        check(query.limit(0).prepare_for_list().slice->limit == 0, "zero limit preserved");
        rejected = false;
        try {
            query.hard_limit(0);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected && query.hard_limit_value == 20'000,
              "hard ceiling must be positive without changing previous ceiling");
        auto& update = context->make<UpdateCommand>("Order", 1u);
        update.expected_version(3).value("name", "new").audit_as("correct name");
        check(update.expected_version_value == 3, "expected version");
        check(update.trace_chain.front().kind == TraceKind::AuditReason, "audit trace");
        check(update.values.fields.find("name")->second.memory_resource() == r,
              "mutation allocator");
        auto& remove = context->make<DeleteCommand>("Order", 1u);
        check(remove.soft_delete && !remove.hard_delete().soft_delete, "soft/hard delete");
        std::cout << "Core contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
