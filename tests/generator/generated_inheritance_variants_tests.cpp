#include "../generated_inheritance_variants/include/cpp_inheritance_variants.hpp"
#include "../generated_inheritance_business_ids/include/cpp_inheritance_business_ids.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void branching_and_retention() {
    using namespace cpp_inheritance_variants;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto executor = setup(*user_context);
    auto* database = user_context->get(executor).transport.connection().native_handle();
    check(
        sqlite3_exec(
            database,
            "UPDATE tenant_data SET name='root',version=3 WHERE id=1;"
            "INSERT INTO document_data(id,name,active,note,tenant,created_at,updated_at,version) VALUES(7,'document',1,NULL,1,111,222,3);"
            "INSERT INTO intermediate_record_data(id,name,active,note,tenant,created_at,updated_at,version) VALUES(7,'intermediate',0,'mid',1,111,222,4);"
            "INSERT INTO invoice_data(id,name,active,note,tenant,created_at,updated_at,version,amount) VALUES(7,'invoice',1,NULL,1,111,222,5,'12.50');"
            "INSERT INTO receipt_data(id,name,active,note,tenant,created_at,updated_at,version,status) VALUES(7,'receipt',0,'branch',1,111,222,6,'open');"
            "INSERT INTO special_invoice_data(id,name,active,note,tenant,created_at,updated_at,version,amount,reference) VALUES(7,'special',1,NULL,1,111,222,7,'20.50','ref');",
            nullptr,
            nullptr,
            nullptr) == SQLITE_OK,
        "seed branched inheritance");
    check(require_value(user_context->require_active_root("Tenant")).get().id == 1,
          "generated initialization prepares the fixture root");
    auto Q = user_context->queries<query_factory>();
    auto query = Q.documentsWithChildren();
    check(query.query().child_enhancements.size() == 2 &&
              query.query().child_enhancements[0].entity == "IntermediateRecord" &&
              query.query()
                      .child_enhancements[0]
                      .child_enhancements[0]
                      .child_enhancements[0]
                      .entity == "SpecialInvoice" &&
              query.query().child_enhancements[1].entity == "Receipt",
          "empty intermediate and sibling branches preserve the recursive tree");
    const auto& one = require_value(std::move(query)
                                        .withIdIs(7)
                                        .purpose("Read branched inheritance")
                                        .comment("Preserve source sibling order")
                                        .executeForOne(*user_context)
                                        .wait());
    check(one && one->name() == "receipt" && !one->active() && one->note() == "branch" &&
              one->dynamic.contains("reference") && one->dynamic.contains("amount"),
          "later sibling fields win while descendant extras remain in the parent view");
    auto& retained = global.make<Document>(*one);
    auto borrowed = E::document(*one);
    user_context.reset();
    bool expired = false;
    try {
        (void)borrowed.getName().unwrap();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired && retained.name() == "receipt" && retained.note() == "branch" &&
              retained.dynamic.get_allocator().resource() == global.memory_resource(),
          "explicit destination copying retains inherited and dynamic data after Context deletion");
}

static void inherited_business_id_cpp_integration() {
    using namespace cpp_inheritance_business_ids;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    setup(*user_context);
    std::array<std::uint8_t, 32> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>(index);
    }
    auto key = require_value(BusinessIdEncodingKey::create(r, 1, bytes));
    auto& provider = user_context->make<StaticBusinessIdKeyProvider>(key);
    require_value(user_context->set_business_id_key_provider(*user_context, provider));
    auto Q = user_context->queries<query_factory>();
    auto root_id = require_value(user_context->require_active_root("Tenant")).get().id;
    auto& invoice = Q.invoices()
                        .purpose("Create inherited business object")
                        .comment("Unqualified original producer combination")
                        .newEntity(*user_context);
    invoice.updateName("invoice")
        .updateActive(false)
        .updateNote(std::optional<String>{})
        .updateAmount(*Decimal::parse("12.50"));
    const auto& saved = require_value(
        AuditedSaveExt<Invoice>::save(
            std::move(invoice).audit_as("Create inherited numbered invoice"), *user_context)
            .wait());
    check(saved.documentNumber().asStr().starts_with("DOC-") && saved.tenantId() == root_id &&
              saved.createdAt().millis > 0 && saved.createdAt() == saved.updatedAt(),
          "one shared inherited Business-ID type works with Context and clock fixes in C++");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        branching_and_retention();
        inherited_business_id_cpp_integration();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Inherited variant C++ integration contracts passed\n";
}
