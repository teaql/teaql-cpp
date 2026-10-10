#include <teaql/core.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void ownership_contracts() {
    GlobalContext global;
    Ref<SelectQuery> retained;
    Ref<String> output;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& query = user->make<SelectQuery>("Order");
        auto& descriptor = user->make<EntityDescriptor>("Child");
        descriptor.property("id", DataType::U64).id();
        descriptor.mask_fields({"private"});
        auto& child = user->make<SelectQuery>("Child");
        child.project("id").filter(Expr::eq(owner, "private", "visible raw diagnostic"));
        query.filter(Expr::in_subquery(owner, "id", descriptor, child, "id"));
        query.relation_query("children", child).child_enhancement(child).comment("diagnostic");
        query.order_gbk_desc("name").sum("amount", "total");
        auto before = user->allocation_stats().issued_objects;
        String qualified(owner);
        core::DebugWriter writer(qualified, {.pretty = true});
        core::write_debug(writer, query);
        check(qualified.find("visible raw diagnostic") != String::npos &&
                  qualified.find("audit_mask_fields") != String::npos,
              "public nested diagnostics retain raw values and complete metadata");
        check(user->allocation_stats().issued_objects == before,
              "formatting scratch consumes no additional object identities");
        auto compact = debug_string(owner, query);
        check(compact == detail::debug_query(owner, query) &&
                  debug_string(owner, *query.predicate) ==
                      detail::debug_expr(owner, *query.predicate),
              "cache-key wrappers retain the default public diagnostic shape");
        retained = user->make_global<SelectQuery>(query);
        output = user->make_global<String>(qualified);
    }
    check(global.live_user_bytes() == 0, "actual User destruction releases all source blocks");
    auto next = global.make_user_context();
    auto& query = next->get(next->rebind_global(retained));
    auto& text = next->get(next->rebind_global(output));
    auto owner = global.memory_resource();
    check(query.memory_resource() == owner && query.predicate->memory_resource() == owner &&
              query.predicate->subquery_data->entity.memory_resource() == owner &&
              query.relations[0].query->memory_resource() == owner &&
              text.get_allocator().resource() == owner,
          "all retained nested query/metadata/output storage uses Global");
    auto repeated = debug_string(next->memory_resource(), query, {.pretty = true});
    check(repeated == text, "complete nested diagnostics survive actual User destruction");
    auto before = next->allocation_stats().issued_objects;
    for (unsigned mode = 0; mode < 3; ++mode) {
        DebugOptions options;
        options.pretty = true;
        options.numbers = DebugNumbers(mode);
        auto formatted = debug_string(next->memory_resource(), query, options);
        check(formatted.find("RelationLoad") != String::npos &&
                  formatted.find("TraceNode") != String::npos,
              "nested formatted query preserves relation and trace abstractions");
    }
    check(next->allocation_stats().issued_objects == before,
          "reformatting in another User does not publish managed objects");
}

template <class T>
static void invalid_enum_contract(Memory owner) {
    try {
        (void)debug_string(owner, T(-1));
        throw std::runtime_error("Invalid diagnostic enum was accepted");
    } catch (const std::invalid_argument&) {
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        ownership_contracts();
        GlobalContext global;
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        invalid_enum_contract<BinaryOp>(owner);
        invalid_enum_contract<ExprFunction>(owner);
        invalid_enum_contract<SortDirection>(owner);
        invalid_enum_contract<AggregateFunction>(owner);
        invalid_enum_contract<TraceKind>(owner);
        Expr malformed(owner, Expr::Kind::SubQuery);
        malformed.args.emplace_back(Expr::column(owner, "id"));
        try {
            (void)debug_string(owner, malformed);
            throw std::runtime_error("Missing subquery metadata was accepted");
        } catch (const std::invalid_argument&) {
        }
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
