#include "../generated_optional_types/include/cpp_optional_types.hpp"
#include "../support/generated_schema_only_fixture.hpp"
#include <iostream>

using namespace teaql;
using namespace cpp_optional_types;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void sqlite_optional_roundtrip_and_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto& module = user_context->make<RuntimeModule>();
    module.entity<Reading>();
    auto& checker = user_context->make<ReadingChecker>();
    auto& adapter =
        user_context->make<TypedEntityChecker<Reading, ReadingChecker>>(*user_context, checker);
    module.checker(*user_context, adapter);
    teaql::test::configure_schema_only<cpp_optional_types::DataServiceExecutor>(*user_context,
                                                                                module);
    check(user_context->has_checker("Reading"), "generated optional Checker is registered");
    auto r = user_context->memory_resource();
    auto Q = user_context->queries<query_factory>();
    auto& reading = Q.readings()
                        .purpose("Create optional scalar fixture")
                        .comment("Exercise generated SQLite assembly")
                        .newEntity(*user_context);
    reading.updateText(std::optional<String>(std::in_place, " \xc2\x85value\xe3\x80\x80 ", r));
    reading.updateActive(false);
    reading.updateBalance(*Decimal::parse("12.5000"));
    reading.updateHeight(0.0);
    reading.updateRatio(0.0f);
    reading.updateCount(std::int64_t(0));
    reading.updateDay(*Date::parse("2024-02-29"));
    reading.updateRecordedAt(Timestamp(-1));
    reading.updatePayload(std::optional<Json>(std::in_place, r));
    auto id = reading.id();
    check(reading.text() && *reading.text() == "value",
          "optional text stores trimmed Unicode text");
    const auto& saved =
        require_value(AuditedSaveExt<Reading>::save(
                          std::move(reading).audit_as("Create optional Reading"), *user_context)
                          .wait());
    check(saved.recordedAt() == Timestamp(-1), "audited save retains optional timestamp");

    auto request =
        Q.readings().withIdIs(id).purpose("Read optional scalar values").comment("SQLite readback");
    const auto& one = require_value(request.executeForOne(*user_context).wait());
    check(one && one->active() == false && one->count() == 0 && one->height() == 0.0 &&
              one->ratio() == 0.0f,
          "false and zero are loaded optional values");
    check(one->balance() == *Decimal::parse("12.5000") &&
              one->day() == *Date::parse("2024-02-29") && one->recordedAt() == Timestamp(-1),
          "optional decimal/date/timestamp values roundtrip through SQLite");
    check(one->payload() && std::holds_alternative<std::monostate>(one->payload()->data) &&
              one->evalPayload().is_value(),
          "stored JSON null remains a present optional JSON value");
    check(E::reading(*one).getActive().unwrap() == false &&
              E::reading(*one).getCount().unwrap() == 0,
          "typed E preserves optional false and zero");
    auto& retained_values = global.make<Reading>(*one);

    auto partial_request = Q.readingsMinimal()
                               .withIdIs(id)
                               .selectRecordedAt()
                               .purpose("Read sparse optional projection")
                               .comment("Identity and timestamp");
    const auto& partial = require_value(partial_request.executeForOne(*user_context).wait());
    check(partial && partial->evalRecordedAt().is_value() && partial->evalText().is_not_loaded(),
          "unselected optional values remain NotLoaded");

    auto& updated = user_context->make<Reading>(*one);
    updated.updateActive(std::nullopt);
    updated.updateBalance(std::nullopt);
    updated.updateRecordedAt(std::nullopt);
    updated.updatePayload(std::nullopt);
    const auto& saved_nulls =
        require_value(AuditedSaveExt<Reading>::save(
                          std::move(updated).audit_as("Clear optional values"), *user_context)
                          .wait());
    check(saved_nulls.evalActive().is_null() && saved_nulls.evalBalance().is_null() &&
              saved_nulls.evalRecordedAt().is_null() && saved_nulls.evalPayload().is_null(),
          "explicit NULL updates preserve loaded Null on audited readback");
    auto null_request = Q.readings()
                            .withIdIs(id)
                            .purpose("Inspect optional NULL persistence")
                            .comment("SQLite readback after clear");
    const auto& nulls = require_value(null_request.executeForOne(*user_context).wait());
    check(nulls && E::reading(*nulls).getRecordedAt().isNull() &&
              E::reading(*nulls).getPayload().isNull(),
          "SQL NULL is distinct from a present JSON null");
    auto& retained_nulls = global.make<Reading>(*nulls);

    auto stream_request = Q.readings()
                              .withIdIs(id)
                              .stream(1)
                              .purpose("Stream optional values")
                              .comment("Generated cursor decode");
    const auto& handle = require_value(stream_request.executeForStream(*user_context).wait());
    auto& stream = handle.get();
    auto* next = stream.wait_next();
    check(next && require_value(*next).evalRecordedAt().is_null() &&
              require_value(*next).count() == 0 && !stream.wait_next(),
          "stream decoding retains NULL and zero availability");

    auto borrowed = E::reading(*one);
    user_context.reset();
    bool expired = false;
    try {
        (void)borrowed.getCount().unwrap();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "generated entity expression expires after actual UserContext deletion");
    check(retained_values.recordedAt() == Timestamp(-1) && retained_values.active() == false &&
              retained_values.payload() &&
              retained_values.payload()->memory_resource() == global.memory_resource(),
          "explicit Global copy retains nested optional data after source deletion");
    check(retained_nulls.evalRecordedAt().is_null() && retained_nulls.evalPayload().is_null(),
          "explicit Global copy preserves loaded optional NULL");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        sqlite_optional_roundtrip_and_lifetimes();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated optional scalar SQLite/lifetime contracts passed\n";
}
