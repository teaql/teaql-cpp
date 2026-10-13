#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace teaql;
using namespace cpp_inheritance_graph;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void inherited_optional_graph_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto executor = setup(*user_context);
    auto* database = user_context->get(executor).transport.connection().native_handle();
    check(
        sqlite3_exec(
            database,
            "UPDATE platform_data SET name='root',version=3 WHERE id=1;"
            "INSERT INTO specialist_data(id,name,platform,active,version) VALUES(2,'advisor',1,1,3);"
            "INSERT INTO research_academy_data(id,name,rank,advisor,platform,version,campus_code,budget) VALUES(7,'research',17,2,1,5,'lab','12.50'),(8,'nullable',18,NULL,1,5,'lab','10.50');",
            nullptr,
            nullptr,
            nullptr) == SQLITE_OK,
        "seed inherited optional graph");
    check(require_value(user_context->require_active_root("Platform")).get().id == 1,
          "generated initialization prepares the fixture root");
    auto Q = user_context->queries<query_factory>();
    auto query = Q.researchAcademies()
                     .selectAdvisorWith(Q.specialists().selectPlatform())
                     .selectAll()
                     .purpose("Read inherited graph")
                     .comment("Two-level optional forward graph");
    const auto& one = require_value(query.withIdIs(7).executeForOne(*user_context).wait());
    check(one && E::researchAcademy(*one).getAdvisor().getPlatform().getName().unwrap() == "root",
          "inherited advisor and nested Platform are explicit loaded views");
    auto& entity = user_context->make<ResearchAcademy>(*one);
    auto& advisor = user_context->make<Specialist>(*entity.advisor());
    entity.updateName("updated root");
    advisor.updateName("updated advisor").updateActive(false);
    advisor.attachRuntimeStateRecursive(entity.root);
    const auto& saved =
        require_value(AuditedSaveExt<ResearchAcademy>::save(
                          std::move(entity).audit_as("Save inherited advisor graph"), *user_context)
                          .wait());
    check(saved.version() == 6 && saved.name() == "updated root",
          "inherited root commits its changed version and scalar");
    const auto& linked = require_value(Q.specialists()
                                           .whoseIdIs(2)
                                           .purpose("Read committed advisor")
                                           .comment("Qualified graph ledger participant")
                                           .executeForOne(*user_context)
                                           .wait());
    check(linked && linked->version() == 4 && !linked->active() &&
              linked->name() == "updated advisor",
          "mutate-before-attach commits the linked participant in the same ledger save");
    const auto& nullable = require_value(Q.researchAcademies()
                                             .withIdIs(8)
                                             .selectAdvisor()
                                             .purpose("Read null inherited advisor")
                                             .comment("Null remains distinct from NotLoaded")
                                             .executeForOne(*user_context)
                                             .wait());
    check(nullable && nullable->evalAdvisor().is_null(), "loaded null advisor evaluates as Null");
    const auto& refreshed = require_value(Q.researchAcademies()
                                              .withIdIs(7)
                                              .selectAdvisorWith(Q.specialists().selectPlatform())
                                              .purpose("Retain inherited graph")
                                              .comment("Explicit destination copy")
                                              .executeForOne(*user_context)
                                              .wait());
    auto& retained = global.make<ResearchAcademy>(*refreshed);
    auto borrowed = E::researchAcademy(*refreshed).getAdvisor().getPlatform();
    user_context.reset();
    bool expired = false;
    try {
        (void)borrowed.getName().unwrap();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "nested inherited graph borrow expires after actual Context deletion");
    check(E::researchAcademy(retained).getAdvisor().getName().unwrap() == "updated advisor" &&
              E::researchAcademy(retained).getAdvisor().getPlatform().getName().unwrap() ==
                  "root" &&
              retained.advisor()->memory_resource() == global.memory_resource(),
          "explicit Global copying retains graph values and destination allocations");
}

static void reverse_collection_save_and_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto executor = setup(*user_context);
    auto* database = user_context->get(executor).transport.connection().native_handle();
    auto sql = [&](const char* command) {
        check(sqlite3_exec(database, command, nullptr, nullptr, nullptr) == SQLITE_OK,
              "reverse collection SQL fixture");
    };
    sql("UPDATE platform_data SET name='root',version=3 WHERE id=1;"
        "INSERT INTO school_data(id,name,rank,advisor,platform,version) VALUES(7,'owner',1,NULL,1,3),(8,'empty',2,NULL,1,3);"
        "INSERT INTO enrollment_data(id,school,note,version) VALUES(21,7,'first',3),(22,7,'second',3);");
    check(require_value(user_context->require_active_root("Platform")).get().id == 1,
          "generated initialization prepares the fixture root");
    auto Q = user_context->queries<query_factory>();
    auto query = Q.schools()
                     .selectEnrollmentList()
                     .purpose("Read reverse collection")
                     .comment("Explicit enrollment selection");
    const auto& one = require_value(query.withIdIs(7).executeForOne(*user_context).wait());
    check(one && one->enrollmentListHandle().state() == LoadedRelation::Loaded &&
              one->enrollmentListHandle().value()->data.size() == 2,
          "reverse view contains both selected members");
    const auto& empty = require_value(Q.schools()
                                          .withIdIs(8)
                                          .selectEnrollmentList()
                                          .purpose("Read empty collection")
                                          .comment("Loaded empty remains a value")
                                          .executeForOne(*user_context)
                                          .wait());
    check(empty && empty->enrollmentListHandle().state() == LoadedRelation::Empty &&
              empty->evalEnrollmentList().is_value(),
          "loaded empty differs from NotLoaded");
    auto& owner = user_context->make<School>(*one);
    owner.updateName("updated owner");
    for (const auto& source : one->enrollmentListHandle().value()->data) {
        auto& child = user_context->make<Enrollment>(source);
        child.updateNote("updated member");
        child.attachRuntimeStateRecursive(owner.root);
    }
    sql("CREATE TRIGGER fail_second BEFORE UPDATE ON enrollment_data WHEN OLD.id=22 BEGIN SELECT RAISE(ABORT,'second member failure'); END;");
    auto& attempt = user_context->make<School>(owner);
    const auto& failed =
        AuditedSaveExt<School>::save(std::move(attempt).audit_as("Fail second member transaction"),
                                     *user_context)
            .wait();
    check(!failed && failed.error().kind() == RuntimeError::Kind::Graph,
          "second member failure rejects the collection ledger");
    const auto& unchanged = require_value(Q.schools()
                                              .withIdIs(7)
                                              .selectEnrollmentList()
                                              .purpose("Inspect rollback")
                                              .comment("All participants remain unchanged")
                                              .executeForOne(*user_context)
                                              .wait());
    check(unchanged && unchanged->name() == "owner" && unchanged->version() == 3 &&
              unchanged->enrollmentListHandle().value()->data[0].note() == "first" &&
              unchanged->enrollmentListHandle().value()->data[1].note() == "second",
          "rollback restores the owner and earlier member writes");
    sql("DROP TRIGGER fail_second;");
    auto& retry = user_context->make<School>(owner);
    const auto& saved = require_value(
        AuditedSaveExt<School>::save(std::move(retry).audit_as("Commit reverse collection ledger"),
                                     *user_context)
            .wait());
    check(saved.version() == 4 && owner.root.current_change_set().changes().empty(),
          "successful retry clears the shared ledger");
    const auto& refreshed = require_value(Q.schools()
                                              .withIdIs(7)
                                              .selectEnrollmentList()
                                              .purpose("Retain reverse graph")
                                              .comment("Explicit destination copy")
                                              .executeForOne(*user_context)
                                              .wait());
    check(refreshed->enrollmentListHandle().value()->data[0].version() == 4 &&
              refreshed->enrollmentListHandle().value()->data[1].version() == 4,
          "both members committed their next versions");
    auto& retained = global.make<School>(*refreshed);
    auto borrowed = E::school(*refreshed).getEnrollmentList().first();
    user_context.reset();
    bool expired = false;
    try {
        (void)borrowed.getNote().unwrap();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "reverse member borrow expires after actual Context deletion");
    const auto& list = retained.enrollmentList();
    check(list.data.size() == 2 && list.data[0].note() == "updated member" &&
              list.data[1].note() == "updated member" &&
              list.data[0].memory_resource() == global.memory_resource(),
          "explicit Global copy preserves the reverse graph and member allocations");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        inherited_optional_graph_lifetimes();
        reverse_collection_save_and_lifetimes();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Inherited optional graph/lifetime contracts passed\n";
}
