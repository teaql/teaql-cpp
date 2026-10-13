#include "../generated_checkers/include/cpp_checker_fixture.hpp"
#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <iostream>
using namespace teaql;
using namespace cpp_checker_fixture;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class NoteLogic final : public NoteCheckerLogic {
public:
    using NoteCheckerLogic::NoteCheckerLogic;
    mutable std::size_t calls = 0;

    void checkAndFixNote(const UserContext& user_context,
                         Note& entity,
                         CheckObjectStatus,
                         const ObjectLocation& location,
                         CheckResults& results) const override {
        ++calls;
        check(entity.memory_resource() == user_context.memory_resource(),
              "logic receives its User Context");
        requiredText(entity.name(), "name", location, results);
    }
};

static void model_rules_and_evidence() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& logic = user_context->make<NoteLogic>();
    auto& checker = user_context->make<NoteChecker>(logic);
    auto& entity = user_context->make<Note>();
    entity.updateId(7).updateVersion(0).updateName("note");
    user_context->with_active_root("Tenant", 42);
    ObjectLocation location(std::allocator_arg, Allocator(r));
    CheckResults results(r);
    auto& create = user_context->make<detail::GraphFixState>(Timestamp(1234));
    {
        detail::GraphFixActivation active(*user_context, create);
        checker.check_and_fix_typed(
            *user_context, entity, CheckObjectStatus::Create, location, results);
    }
    check(
        results.empty() && entity.tenantId() == 42 && entity.createdAt() == Timestamp(1234) &&
            entity.updatedAt() == Timestamp(1234) && create.evidence.size() == 4 &&
            logic.calls == 1,
        "generated Context/create/update fixes precede business logic and preserve source evidence multiplicity");
    auto& update = user_context->make<detail::GraphFixState>(Timestamp(5678));
    {
        detail::GraphFixActivation active(*user_context, update);
        checker.check_and_fix_typed(
            *user_context, entity, CheckObjectStatus::Update, location, results);
    }
    check(results.empty() && entity.createdAt() == Timestamp(1234) &&
              entity.updatedAt() == Timestamp(5678) && update.evidence.size() == 1,
          "update fixes preserve creation time and use the operation clock");
    entity.root.set(entity.entity_key(), "tenant_id", Value(r, std::uint64_t(43)));
    checker.check_and_fix_typed(
        *user_context, entity, CheckObjectStatus::Update, location, results);
    check(results.size() == 1 && results.front().rule == CheckRule::ContextRootMismatch,
          "generated update checks reject mismatched Context roots");
    results.clear();
    auto& partial = user_context->make<Note>();
    partial.updateId(8).updateVersion(1).updateName("partial");
    checker.check_and_fix_typed(
        *user_context, partial, CheckObjectStatus::Update, location, results);
    check(results.size() == 2 && results[0].rule == CheckRule::InvalidType &&
              results[1].rule == CheckRule::InvalidType,
          "missing predefined tenant/create fields reject sparse updates after update-clock fix");
}

static void registration_and_real_save() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    setup(*user_context);
    check(user_context->has_checker("Tenant") && user_context->has_checker("Note"),
          "setup registers actual generated typed checkers");
    auto Q = user_context->queries<query_factory>();
    auto id = require_value(user_context->require_active_root("Tenant")).get().id;
    auto& note = Q.notes()
                     .purpose("Exercise generated fixes")
                     .comment("No root or times supplied")
                     .newEntity(*user_context);
    note.updateName("fixed note");
    require_value(AuditedSaveExt<Note>::save(std::move(note).audit_as("Create note with fixes"),
                                             *user_context)
                      .wait());
    auto evidence = user_context->last_fix_evidence();
    if (evidence.size() != 4) {
        throw std::runtime_error("fix evidence count=" + std::to_string(evidence.size()));
    }
    auto request = Q.notes().purpose("Read generated fix results").comment("SQLite readback");
    auto& rows = require_value(request.executeForList(*user_context).wait());
    check(rows.len() == 1 && rows[0].tenantId() == id && rows[0].createdAt() == rows[0].updatedAt(),
          "actual SQLite save writes the trusted root and common operation timestamps");
}

int main() {
    auto* prior = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        model_rules_and_evidence();
        registration_and_real_save();
        std::pmr::set_default_resource(prior);
        std::cout << "Generated Checker integration passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(prior);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
