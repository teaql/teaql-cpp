#include "../../tests/generated_checkers/include/cpp_checker_fixture.hpp"
#include <iostream>

using namespace teaql;
using namespace cpp_checker_fixture;

template <class T>
concept PublicContextIdSetter = requires(T& value) { value.updateTenantId(std::uint64_t(42)); };

template <class T>
concept PublicContextObjectSetter =
    requires(T& value, const Tenant& root) { value.updateTenant(root); };

static_assert(!PublicContextIdSetter<Note>);
static_assert(!PublicContextObjectSetter<Note>);

int main() {
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        setup(*user_context);
        auto Q = user_context->queries<query_factory>();
        auto id = require_value(user_context->require_active_root("Tenant")).get().id;
        auto& note = Q.notes()
                         .purpose("Create module fixture note")
                         .comment("Generated internal assignment")
                         .newEntity(*user_context);
        note.updateName("module note");
        const auto& saved =
            require_value(AuditedSaveExt<Note>::save(std::move(note).audit_as("Create module note"),
                                                     *user_context)
                              .wait());
        if (saved.tenantId() != id || saved.createdAt() != saved.updatedAt()) {
            throw std::runtime_error(
                "Generated checker must retain Context-root and clock assignment");
        }
        const auto& counts = require_value(Q.notesMinimal()
                                               .groupByTenantAs("tenant_group")
                                               .countIdAs("note_count")
                                               .comment("Count saved notes by tenant")
                                               .purpose("Qualify generated aggregate module API")
                                               .executeForRows(*user_context)
                                               .wait());
        if (counts.data.size() != 1 || !counts.data[0].get("note_count") ||
            counts.data[0].get("note_count")->try_u64() != std::optional<std::uint64_t>(1)) {
            throw std::runtime_error(
                "Module generated aggregate API must return actual compact metrics");
        }
        const auto& tenants =
            require_value(Q.tenants()
                              .countNotesAs("_note_count")
                              .comment("Count tenant relation members")
                              .purpose("Qualify generated relationship module API")
                              .executeForList(*user_context)
                              .wait());
        if (tenants.data.size() != 1 || tenants.data[0].dynamic.at("_note_count").try_u64() !=
                                            std::optional<std::uint64_t>(1)) {
            throw std::runtime_error(
                "Module relationship aggregation must attach an actual metric");
        }
        const auto& faceted =
            require_value(Q.notes()
                              .facetByTenantAsWithOptions(
                                  "_tenants",
                                  Q.tenants().comment("Facet tenant counts").countNotesAs("_notes"),
                                  false)
                              .comment("Inspect module facets")
                              .purpose("Qualify generated facet module API")
                              .executeForList(*user_context)
                              .wait());
        const auto* facet = faceted.facet("_tenants");
        if (!facet || facet->data.size() != 1 ||
            facet->data[0].get("_notes")->try_u64() != std::optional<std::uint64_t>(1)) {
            throw std::runtime_error("Module facets must attach actual relation counts");
        }
        std::cout << "Module Context-root API and save contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
