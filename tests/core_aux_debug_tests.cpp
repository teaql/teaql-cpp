#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;
using namespace teaql::dynamic_search;

static_assert(noexcept(std::declval<const DecimalError&>().source()));

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct AuxiliaryModels {
    using allocator_type = Allocator;
    BusinessIdDefinition definition;
    BusinessIdScope scope;
    BusinessIdGenerationRequest request;
    BusinessIdPlan plan;
    BusinessIdAllocation allocation;
    BusinessIdValue id;
    BusinessIdError business_error;
    SearchModel model;
    NormalizedDynamicSearch normalized;
    DynamicSearchResult result;
    DecimalError decimal_error;

    AuxiliaryModels(std::allocator_arg_t, Allocator a, std::string_view text)
        : definition(std::allocator_arg, a, "field", text, "namespace"),
          scope(std::allocator_arg, a, text, "Aggregate", "namespace", text),
          request(std::allocator_arg, a, definition, text, "Aggregate", *Date::parse("2024-02-29")),
          plan(std::allocator_arg, a, scope, text, text, 6, "-", 0, UINT64_MAX),
          allocation(std::allocator_arg, a, scope, UINT64_MAX), id(std::allocator_arg, a, text),
          business_error(std::allocator_arg, a, BusinessIdErrorCode::Allocation, text),
          model(std::allocator_arg, a), normalized(std::allocator_arg, a),
          result(std::allocator_arg,
                 a,
                 SelectQuery(std::allocator_arg, a, text),
                 std::span<const DynamicSearchWarning>{}),
          decimal_error(std::allocator_arg, a, DecimalError::Code::ErrorString, text) {
        auto owner = a.resource();
        model.fields.emplace(String(text, owner), String(text, owner));
        model.relations.emplace(String(text, owner), String(text, owner));
        Json::Object payload(owner);
        payload.emplace(String(text, owner), Json(owner, text));
        normalized.filters.emplace_back(text, "$eq", Json(owner, std::move(payload)));
        normalized.orders.emplace_back(text, true);
        normalized.warnings.emplace_back(text, "filter", text);
        result.query.comment(text);
        result.warnings.emplace_back(normalized.warnings.front());
    }

    AuxiliaryModels(std::allocator_arg_t, Allocator a, const AuxiliaryModels& other)
        : definition(std::allocator_arg, a, other.definition),
          scope(std::allocator_arg, a, other.scope), request(std::allocator_arg, a, other.request),
          plan(std::allocator_arg, a, other.plan),
          allocation(std::allocator_arg, a, other.allocation), id(std::allocator_arg, a, other.id),
          business_error(std::allocator_arg, a, other.business_error),
          model(std::allocator_arg, a, other.model),
          normalized(std::allocator_arg, a, other.normalized),
          result(std::allocator_arg, a, other.result),
          decimal_error(std::allocator_arg, a, other.decimal_error) {}
};

static String all_diagnostics(Memory owner, const AuxiliaryModels& value) {
    String output(owner);
    DebugWriter writer(output, {.pretty = true});
    core::write_debug(writer, value.definition);
    write_debug(writer, value.scope);
    write_debug(writer, value.request);
    write_debug(writer, value.plan);
    write_debug(writer, value.allocation);
    write_debug(writer, value.id);
    write_debug(writer, value.business_error);
    core::write_debug(writer, value.model);
    write_debug(writer, value.normalized);
    write_debug(writer, value.result);
    core::write_debug(writer, value.decimal_error);
    write_debug(writer, UnpackedDecimal{true, UINT32_MAX, UINT32_MAX, 1, 0});
    write_debug(writer, RoundingStrategy::BankersRounding);
    write_debug(writer, DynamicSearchError{"Invalid trusted search setup"});
    return output;
}

static void retention_and_allocator_contracts() {
    GlobalContext global;
    Ref<AuxiliaryModels> retained;
    Ref<String> expected;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        String text(8192, 'p', owner);
        auto& models = user->make<AuxiliaryModels>(text);
        auto before = user->allocation_stats().issued_objects;
        auto diagnostic = all_diagnostics(owner, models);
        check(user->allocation_stats().issued_objects == before &&
                  diagnostic.find("namespace_name") == String::npos &&
                  diagnostic.find("operator_name") == String::npos &&
                  diagnostic.find(text) != String::npos &&
                  diagnostic.find("ScaleExceeds") == String::npos,
              "raw diagnostics preserve source labels and full text without object identities");
        retained = user->make_global<AuxiliaryModels>(models);
        expected = user->make_global<String>(diagnostic);
    }
    check(global.live_user_bytes() == 0, "actual User deletion returns every source block");
    auto next = global.make_user_context();
    const auto& models = next->get(next->rebind_global(retained));
    auto owner = global.memory_resource();
    const auto& field = *models.model.fields.begin();
    const auto& filter = models.normalized.filters.front();
    const auto& payload = *std::get<Json::Object>(filter.value.data).begin();
    check(
        models.request.definition.memory_resource() == owner &&
            models.plan.scope.memory_resource() == owner &&
            models.allocation.scope.memory_resource() == owner &&
            models.id.memory_resource() == owner &&
            models.business_error.memory_resource() == owner &&
            models.decimal_error.memory_resource() == owner &&
            field.first.get_allocator().resource() == owner &&
            field.second.get_allocator().resource() == owner &&
            filter.field_path.get_allocator().resource() == owner &&
            payload.first.get_allocator().resource() == owner &&
            std::get<String>(payload.second.data).get_allocator().resource() == owner &&
            models.result.query.memory_resource() == owner &&
            models.result.warnings.front().entity.get_allocator().resource() == owner,
        "retained models own every nested definition/map/query/warning/JSON/error payload in Global");
    check(all_diagnostics(next->memory_resource(), models) ==
              next->get(next->rebind_global(expected)),
          "complete nested diagnostics remain identical after actual source destruction");
}

static void decimal_error_source_contracts() {
    GlobalContext global;
    std::array<Ref<DecimalError>, 6> retained;
    {
        auto user = global.make_user_context();
        String payload(8192, 'e', user->memory_resource());
        for (unsigned i = 0; i < retained.size(); ++i) {
            auto& error =
                user->make<DecimalError>(static_cast<DecimalError::Code>(i), payload, UINT32_MAX);
            check(error.source() == nullptr, "every original Decimal error has no nested cause");
            retained[i] = user->make_global<DecimalError>(error);
        }
    }
    check(global.live_user_bytes() == 0, "Decimal errors retain no source User blocks");
    auto user = global.make_user_context();
    auto& unicode = user->make<DecimalError>(DecimalError::Code::ErrorString, "a\u96eab\U0001f9ca");
    check(unicode.format(
              user->memory_resource(),
              {.alignment = DecimalAlignment::Center, .precision = 2, .width = 5, .fill = U'_'}) ==
              "_a\u96ea__",
          "error Display truncates and pads by Unicode characters, preserving complete UTF-8");
    check(unicode.format(user->memory_resource(), {.precision = 0}).empty(),
          "zero Display precision removes the ErrorString payload");
    for (unsigned i = 0; i < retained.size(); ++i) {
        const auto& error = user->get(user->rebind_global(retained[i]));
        check(error.code == static_cast<DecimalError::Code>(i) && error.source() == nullptr &&
                  error.memory_resource() == global.memory_resource() &&
                  error.text.size() == 8192 && error.scale == UINT32_MAX,
              "explicit Global copies preserve the error and cause contract after User deletion");
        auto formatted = error.format(user->memory_resource(), {.precision = 1, .width = 12});
        check(
            formatted.get_allocator().resource() == user->memory_resource() &&
                (i == 0 ? formatted == "           e"
                        : formatted == error.to_string(user->memory_resource())),
            "ErrorString alone observes outer Display flags; other variants preserve write! output");
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        retention_and_allocator_contracts();
        decimal_error_source_contracts();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
