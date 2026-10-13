#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
using namespace cpp_inheritance;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void source_and_result_are_independent() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto executor = setup(*user_context);
    auto* database = user_context->get(executor).transport.connection().native_handle();
    const auto* sql =
        "UPDATE platform_data SET name='root',version=3 WHERE id=1;"
        "INSERT INTO school_data(id,name,rank,platform,version) VALUES(7,'school',1,1,3),(9,'plain',3,1,3);"
        "INSERT INTO academy_data(id,name,rank,platform,version,campus_code) VALUES(7,'academy',7,1,4,'north');"
        "INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'research',17,1,5,'lab','12.50');";
    if (sqlite3_exec(database, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    check(require_value(user_context->require_active_root("Platform")).get().id == 1,
          "generated initialization prepares the fixture root");
    auto Q = user_context->queries<query_factory>();
    auto source = Q.schoolsWithChildren();
    auto adapted = source.returnType<ResearchAcademy>();
    adapted.withIdIs(7);
    check(adapted.query().entity == "School" && !source.query().predicate,
          "lvalue adaptation preserves source identity and owns independent query state");
    auto executable =
        std::move(adapted).purpose("Decode descendant model").comment("Source is School");
    const auto& rows = require_value(executable.executeForList(*user_context).wait());
    static_assert(std::same_as<std::remove_cvref_t<decltype(rows.data.front())>, ResearchAcademy>);
    check(rows.len() == 1 && rows[0].name() == "research" && rows[0].campusCode() == "lab" &&
              rows[0].budget() == *Decimal::parse("12.50"),
          "parent-table query decodes the requested descendant after recursive enhancement");
    const auto& sparse = require_value(Q.schoolsMinimal()
                                           .withIdIs(9)
                                           .returnType<ResearchAcademy>()
                                           .purpose("Decode sparse result")
                                           .comment("No descendant row")
                                           .executeForOne(*user_context)
                                           .wait());
    check(sparse && sparse->id() == 9 && sparse->evalName().is_not_loaded() &&
              sparse->evalBudget().is_not_loaded(),
          "return adaptation does not invent selected fields or descendant values");
    auto again = executable.returnType<Academy>();
    check(require_value(again.executeForOne(*user_context).wait())->campusCode() == "lab",
          "purpose state survives further return adaptation");
    static_assert(std::same_as<decltype(again.newEntity(*user_context)), School&>);
    auto& created = again.newEntity(*user_context);
    check(created.is_new(), "adapted creation still constructs the source model");
    auto next_user_context = global.make_user_context();
    auto copied = again.copyTo(*next_user_context);
    check(copied.query().entity == "School" &&
              copied.memory_resource() == next_user_context->memory_resource(),
          "destination copy retains source/result separation");
    auto retained = again.copyToGlobal(*user_context);
    auto expired = source.returnType<ResearchAcademy>();
    auto& retained_model = global.make<ResearchAcademy>(rows[0]);
    auto expression = E::researchAcademy(rows[0]);
    user_context.reset();
    bool rejected = false;
    try {
        (void)expired.query();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "adapted request expires after actual Context deletion");
    auto rebound = retained.rebindGlobal(*next_user_context);
    check(rebound.query().entity == "School" && retained_model.campusCode() == "lab",
          "explicit Global request/model retention survives the source Context");
    rejected = false;
    try {
        (void)expression.getBudget().unwrap();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "adapted entity expression remains a checked Context borrow");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        source_and_result_are_independent();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated source/result query contracts passed\n";
}
