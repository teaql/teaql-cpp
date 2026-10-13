#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace teaql;
using namespace cpp_inheritance;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void inherited_storage_and_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    setup(*user_context);
    auto Q = user_context->queries<query_factory>();
    auto id = require_value(user_context->require_active_root("Platform")).get().id;
    auto& entity = Q.researchAcademies()
                       .purpose("Create inherited model")
                       .comment("Two-level inherited fields")
                       .newEntity(*user_context);
    entity.updateName("research")
        .updateRank(7)
        .updateCampusCode("north")
        .updateBudget(*Decimal::parse("12.50"));
    auto entity_id = entity.id();
    const auto& saved =
        require_value(AuditedSaveExt<ResearchAcademy>::save(
                          std::move(entity).audit_as("Create research academy"), *user_context)
                          .wait());
    check(saved.name() == "research" && saved.rank() == 7 && saved.campusCode() == "north" &&
              saved.budget() == *Decimal::parse("12.50") && saved.platformId() == id,
          "inherited scalar/foreign fields persist through generated Checker and audited save");
    auto request = Q.researchAcademies()
                       .withIdIs(entity_id)
                       .withRankIs(7)
                       .purpose("Read inherited model")
                       .comment("Typed inherited query predicates");
    const auto& one = require_value(request.executeForOne(*user_context).wait());
    check(one && E::researchAcademy(*one).getName().unwrap() == "research" &&
              E::researchAcademy(*one).getCampusCode().unwrap() == "north",
          "inherited typed query/E surface reads both ancestor levels");
    auto sparse = Q.researchAcademiesMinimal()
                      .withIdIs(entity_id)
                      .selectCampusCode()
                      .purpose("Read sparse inherited model")
                      .comment("Do not load parent name");
    const auto& partial = require_value(sparse.executeForOne(*user_context).wait());
    check(partial && partial->evalName().is_not_loaded() && partial->evalCampusCode().is_value(),
          "inherited fields retain independent indexed availability");
    auto& retained = global.make<ResearchAcademy>(*one);
    auto& deleting = user_context->make<ResearchAcademy>(*one);
    check(&deleting.markForDeletion() == &deleting && deleting.is_marked_as_delete(),
          "application deletion returns the model and records a pending deletion");
    const auto& before_delete = require_value(request.executeForOne(*user_context).wait());
    check(before_delete && before_delete->version() == 1, "marking alone does not execute SQL");
    const auto& deleted =
        require_value(AuditedSaveExt<ResearchAcademy>::save(
                          std::move(deleting).audit_as("Delete inherited model"), *user_context)
                          .wait());
    check(deleted.version() == -2 && !deleted.is_marked_as_delete(),
          "audited soft deletion increments and negates the version, then clears its marker");
    const auto& after_delete = require_value(request.executeForOne(*user_context).wait());
    check(!after_delete, "normal inherited queries exclude the deleted row after commit");
    auto borrowed = E::researchAcademy(*one);
    user_context.reset();
    bool expired = false;
    try {
        (void)borrowed.getName().unwrap();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "inherited model borrow expires after actual UserContext deletion");
    check(retained.name() == "research" && retained.campusCode() == "north" &&
              retained.platformId() == id &&
              retained.name().get_allocator().resource() == global.memory_resource(),
          "explicit Global copy retains complete inherited state and destination allocations");
}

static void inherited_stream_context_cleanup() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto install = [&](UserContext& user_context) {
        auto& module = moduleWithBehaviorsAndCheckers(user_context);
        module.apply_to(user_context);
        user_context.set_metadata(user_context, module.metadata);
        use_sqlite_provider(user_context, global, transport);
        auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
        user_context.set_internal_id_generator(user_context, ids);
        auto dialect = user_context.require_resource<SqliteDialect>();
        auto& state = user_context.make<SqlExecutorState>();
        auto executor = user_context.make_ref<cpp_inheritance::DataServiceExecutor>(
            require_value(dialect).get(), transport, module.metadata, state);
        user_context.register_executor(executor);
        require_value(user_context.ensure_schema().wait());
        check(require_value(user_context.require_active_root("Platform")).get().id == 1,
              "manual module assembly prepares its generated root");
    };
    auto user_context = global.make_user_context();
    install(*user_context);
    check(
        sqlite3_exec(
            connection.native_handle(),
            "UPDATE platform_data SET name='root',version=3 WHERE id=1;"
            "INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'first',1,1,3,'north','12.50'),(8,'second',2,1,3,'south','20.50');",
            nullptr,
            nullptr,
            nullptr) == SQLITE_OK,
        "seed inherited stream cleanup fixture");
    auto Q = user_context->queries<query_factory>();
    const auto& stream_result = require_value(Q.researchAcademies()
                                                  .orderByIdAsc()
                                                  .stream(1)
                                                  .purpose("Read inherited stream")
                                                  .comment("Destroy Context with a live cursor")
                                                  .executeForStream(*user_context)
                                                  .wait());
    using StreamBorrow = std::remove_cvref_t<decltype(stream_result)>;
    StreamBorrow stream_borrow(
        std::allocator_arg, Allocator(user_context->memory_resource()), stream_result);
    auto* first = stream_borrow.get().wait_next();
    check(first && *first && first->value().name() == "first",
          "first inherited stream item is decoded");
    auto expression = E::researchAcademy(first->value());
    auto& retained = global.make<ResearchAcademy>(first->value());
    auto& pending = Q.researchAcademies()
                        .withIdIs(7)
                        .purpose("Observe held stream lease")
                        .comment("A second query must wait for the live cursor")
                        .executeForOne(*user_context);
    check(!pending.poll(), "live inherited stream holds the SQLite lease");
    user_context.reset();
    bool expired_stream = false;
    bool expired_expression = false;
    try {
        (void)stream_borrow.get();
    } catch (const ReferenceError&) {
        expired_stream = true;
    }
    try {
        (void)expression.getName().unwrap();
    } catch (const ReferenceError&) {
        expired_expression = true;
    }
    check(
        expired_stream && expired_expression && retained.name() == "first" &&
            retained.campusCode() == "north" &&
            retained.name().get_allocator().resource() == global.memory_resource(),
        "Context destruction invalidates checked stream/model borrows and retains only explicit Global copies");
    auto next_user_context = global.make_user_context();
    install(*next_user_context);
    auto next_Q = next_user_context->queries<query_factory>();
    auto& reuse = next_Q.researchAcademies()
                      .withIdIs(8)
                      .purpose("Reuse connection after Context destruction")
                      .comment("Old stream and queued operation have been cancelled")
                      .executeForOne(*next_user_context);
    const auto* result = reuse.poll();
    check(result && *result && result->value() && result->value()->name() == "second",
          "Context destruction releases active and queued SQLite leases before arena reclamation");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        inherited_storage_and_lifetimes();
        inherited_stream_context_cleanup();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Qualification-only inherited storage/lifetime contracts passed\n";
}
