#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include <iostream>

using namespace teaql;
using namespace cpp_business_ids;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class T>
concept StringBusinessIdUpdate = requires(T& model) { model.updateReceiptNumber("text"); };
static_assert(!StringBusinessIdUpdate<Receipt>);
static_assert(!std::same_as<ReceiptNumber, TicketNumber>);

static void value_types_and_lifetimes() {
    GlobalContext global;
    auto& retained = [&]() -> ReceiptNumber& {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto value = ReceiptNumber::create(r, " \t\u3000RCP-20261011-ABC123\u00a0 ");
        check(value && value.value().asStr() == "RCP-20261011-ABC123",
              "typed creation trims Unicode whitespace");
        auto blank = ReceiptNumber::create(r, " \u3000\t");
        check(!blank && blank.error() == "receipt_number must not be blank",
              "blank typed values retain the source error");
        auto canonical = ReceiptNumber::create(r, "not a formatted ID");
        check(bool(canonical),
              "value construction leaves profile validation to the registered service");
        auto& entity = user_context->make<Receipt>();
        check(entity.receiptNumber().asStr().empty() && entity.evalReceiptNumber().is_not_loaded(),
              "empty storage does not manufacture loaded Business IDs");
        entity.updateReceiptNumber(value.value());
        check(entity.changedReceiptNumber()->try_text() == value.value().asStr() &&
                  entity.evalReceiptNumber().get().asStr() == value.value().asStr() &&
                  E::receipt(entity).getReceiptNumber().unwrap().asStr() == value.value().asStr(),
              "model mutation and E retain typed values over text storage");
        auto& alias = user_context->make<Receipt>(entity);
        auto replacement = ReceiptNumber::create(r, "replacement");
        entity.updateReceiptNumber(replacement.value());
        check(alias.receiptNumber().asStr() == "replacement",
              "typed getters observe the shared ledger");
        return global.make<ReceiptNumber>(value.value());
    }();
    check(retained.asStr() == "RCP-20261011-ABC123" &&
              retained.memory_resource() == global.memory_resource(),
          "typed Global copy survives actual User destruction");
    auto& copy = global.make<ReceiptNumber>(retained);
    check(copy == retained &&
              std::hash<ReceiptNumber>{}(copy) == std::hash<ReceiptNumber>{}(retained),
          "typed equality and hashing ignore allocation identity");
}

static void registration_and_real_save() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    setup(*user_context);
    check(bool(user_context->business_ids()),
          "generated setup installs persistent SQLite Business-ID infrastructure");
    std::array<std::uint8_t, 32> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(i);
    }
    auto key = BusinessIdEncodingKey::create(r, 1, bytes);
    auto& provider = user_context->make<StaticBusinessIdKeyProvider>(key.value());
    require_value(user_context->set_business_id_key_provider(*user_context, provider));
    auto Q = user_context->queries<query_factory>();
    auto root_id = require_value(user_context->require_active_root("Tenant")).get().id;
    auto& receipt = Q.receipts()
                        .purpose("Allocate Business ID")
                        .comment("No supplied number")
                        .newEntity(*user_context);
    receipt.updateName("receipt");
    require_value(AuditedSaveExt<Receipt>::save(
                      std::move(receipt).audit_as("Create numbered receipt"), *user_context)
                      .wait());
    auto evidence = user_context->last_fix_evidence();
    check(evidence.size() == 2 && evidence.back().source_label == "businessIdAllocator",
          "real create retains root and Business-ID fix evidence");
    auto& rows = require_value(Q.receipts()
                                   .purpose("Read generated number")
                                   .comment("SQLite readback")
                                   .executeForList(*user_context)
                                   .wait());
    check(rows.len() == 1 && rows[0].receiptNumber().asStr().starts_with("RCP-"),
          "SQLite hydration exposes the distinct value type");
    auto number = rows[0].receiptNumber();
    auto& filtered = require_value(Q.receipts()
                                       .withReceiptNumberIs(number)
                                       .purpose("Find numbered receipt")
                                       .comment("Strong typed predicate")
                                       .executeForList(*user_context)
                                       .wait());
    check(filtered.len() == 1, "typed predicates encode their value in the query Context");
    auto replacement = ReceiptNumber::create(r, "RCP-20990101-ABC123");
    auto& changed = user_context->make<Receipt>(rows[0]);
    changed.updateReceiptNumber(replacement.value());
    auto& saved = AuditedSaveExt<Receipt>::save(
                      std::move(changed).audit_as("Attempt to change number"), *user_context)
                      .wait();
    check(!saved, "governed writes reject a dirty Business ID after create");
    auto& unchanged = require_value(Q.receipts()
                                        .purpose("Verify rejected update")
                                        .comment("Business ID remains unchanged")
                                        .executeForList(*user_context)
                                        .wait());
    check(unchanged[0].receiptNumber().asStr() == number.asStr(),
          "immutable rejection precedes the protected SQLite update");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        value_types_and_lifetimes();
        registration_and_real_save();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated Business-ID contracts passed\n";
}
