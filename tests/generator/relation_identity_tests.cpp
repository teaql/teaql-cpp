#include "parity/relation_identity_contracts/cpp/include/cpp_relation_identities.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

using namespace cpp_relation_identities;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class Expression>
static void
missing(const Expression& expression, std::string_view identity, std::string_view suggestion) {
    try {
        (void)expression.unwrap();
    } catch (const ExpressionNotLoadedError& error) {
        const auto& diagnostic = error.diagnostic();
        check(diagnostic.failed_node == identity && diagnostic.missing_preload[0] == identity &&
                  diagnostic.suggested_fix == suggestion,
              "Missing-load diagnostics must retain the canonical relationship path");
        return;
    }
    throw std::runtime_error("Expected a missing-load diagnostic");
}

static auto load_entry(UserContext& user_context, auto request) {
    const auto& one = require_value(std::move(request)
                                        .comment("Inspect keyword relationship")
                                        .purpose("Verify a diagnostic repair executes")
                                        .executeForOne(user_context)
                                        .wait());
    check(bool(one), "The expected Entry row must exist");
    return EntityBorrow<Entry>(*one);
}

static void generated_relationships() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    std::optional<EntryExpression> expired;
    Ref<Entry> retained;
    {
        auto connection = user_context->make_resource_ref<SqliteConnection>();
        auto& sqlite = user_context->get(connection);
        std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                           "/tests/support/relation_identity_sql.sql");
        std::ostringstream sql;
        sql << file.rdbuf();
        check(bool(file) &&
                  sqlite3_exec(
                      sqlite.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) ==
                      SQLITE_OK,
              "Initialize the retained SQLite fixture");
        require_value(serviceRuntimeFromConnection(*user_context, connection));
        user_context->with_active_root("Platform", 1);
        auto Q = user_context->queries<query_factory>();
        auto sparse = load_entry(*user_context, Q.entriesMinimal().withIdIs(20));
        auto expression = E::entry(sparse.get());
        missing(expression.getNamespace(), "namespace", "selectNamespace()");
        missing(expression.getModelNamespace(), "namespace", "selectNamespace()");
        missing(expression.getOperator(), "operator", "selectOperator()");
        auto loaded = load_entry(*user_context,
                                 Q.entriesMinimal()
                                     .withIdIs(20)
                                     .selectNamespaceWith(Q.owners().selectName())
                                     .selectOperatorWith(Q.owners().selectName()));
        check(E::entry(loaded.get()).getNamespace().getName().unwrap() == "Alpha" &&
                  E::entry(loaded.get()).getModelNamespace().getName().unwrap() == "Alpha" &&
                  E::entry(loaded.get()).getOperator().getName().unwrap() == "Beta",
              "Canonical diagnostic repair delegates to the existing Context-bound query");
        auto removed = load_entry(
            *user_context, Q.entriesMinimal().withIdIs(20).selectNamespace().unselectNamespace());
        missing(E::entry(removed.get()).getNamespace(), "namespace", "selectNamespace()");
        auto null_entry = load_entry(*user_context, Q.entries().withIdIs(21).selectOperator());
        check(E::entry(null_entry.get()).getOperator().isNull(),
              "An optional keyword relation retains loaded NULL");
        const auto& owners =
            require_value(Q.ownersMinimal()
                              .withIdIs(10)
                              .selectClassWith(Q.entries().selectName())
                              .selectTemplateWith(Q.entries().selectName())
                              .selectOperator()
                              .comment("Read reverse keyword relations")
                              .purpose("Verify canonical one/many relationship access")
                              .executeForList(*user_context)
                              .wait());
        check(owners.len() == 1 && E::owner(owners[0]).getClass().size().unwrap() == 2 &&
                  E::owner(owners[0]).getTemplate().first().getName().unwrap() == "Other" &&
                  E::owner(owners[0]).getOperator().getName().unwrap() == "Alpha profile",
              "Reverse list and one-to-one canonical aliases retain real graph views");
        expired.emplace(E::entry(loaded.get()));
        retained = user_context->make_global<Entry>(loaded.get());
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Actual User deletion reclaims all registered storage");
    bool rejected = false;
    try {
        (void)expired->getNamespace().getName().unwrap();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Canonical E wrappers must not extend the source Context lifetime");
    auto destination = global.make_user_context();
    const auto& copied = destination->get(destination->rebind_global(retained));
    check(E::entry(copied).getNamespace().getName().unwrap() == "Alpha",
          "Explicit Global copy retains the canonical relationship graph");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        generated_relationships();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
