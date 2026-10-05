#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static Json error(Memory r, const BusinessIdError& e) {
    auto out = detail::model_json(r);
    detail::model_put(out, "code", business_id_error_code_name(e.code));
    detail::model_put(out, "message", e.message);
    detail::model_put(out, "display", e.to_string(r));
    return out;
}

static String numeric(Memory r, std::int64_t n) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), n).ptr;
    return String(buffer, end, r);
}

static String unsigned_numeric(Memory r, std::uint64_t n) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), n).ptr;
    return String(buffer, end, r);
}

static std::optional<Date> date(const Json& j) {
    auto* s = std::get_if<String>(&j.data);
    return s ? Date::parse(*s) : std::nullopt;
}

static BusinessIdScope scope(Memory r, const Json& j) {
    auto& a = std::get<Json::Array>(j.data);
    return BusinessIdScope(
        std::allocator_arg, Allocator(r), text(a[0]), text(a[1]), text(a[2]), text(a[3]));
}

static BusinessIdPlan plan(Memory r, const Json& j) {
    return BusinessIdPlan(std::allocator_arg,
                          Allocator(r),
                          scope(r, field(j, "scope")),
                          "P",
                          std::nullopt,
                          6,
                          "-",
                          number<std::uint64_t>(field(j, "initial")),
                          number<std::uint64_t>(field(j, "maximum")));
}

static BusinessIdDefinition definition(Memory r, const Json& j) {
    BusinessIdDefinition d(std::allocator_arg,
                           Allocator(r),
                           text(field(j, "field")),
                           text(field(j, "prefix")),
                           text(field(j, "namespace")));
    d.profile = text(field(j, "profile"));
    d.separator = text(field(j, "separator"));
    d.split_by_date = std::get<bool>(field(j, "split").data);
    d.preserve_digits = std::uint8_t(std::get<std::uint64_t>(field(j, "digits").data));
    return d;
}

struct Clock : BusinessClock {
    UtcDateTime time;
    std::optional<Date> date;
    Vector<String>& log;

    Clock(UtcDateTime time, std::optional<Date> date, Vector<String>& log)
        : time(time), date(date), log(log) {}

    UtcDateTime business_time() const override {
        log.emplace_back("time");
        return time;
    }

    Date business_date() const override {
        log.emplace_back("date");
        return date ? *date : business_time().date_naive();
    }
};

struct Key : BusinessIdKeyProvider {
    using allocator_type = Allocator;
    String mode;
    Vector<String>& log;

    Key(std::allocator_arg_t, Allocator a, std::string_view mode, Vector<String>& log)
        : mode(mode, a.resource()), log(log) {}

    Memory memory_resource() const {
        return mode.get_allocator().resource();
    }

    Result<BusinessIdEncodingKey, BusinessIdError>
    current_key(Memory r, const UserContext&, const BusinessIdDefinition&) const override {
        log.emplace_back("key");
        if (mode == "error") {
            return Result<BusinessIdEncodingKey, BusinessIdError>::err(
                r,
                BusinessIdError(std::allocator_arg,
                                Allocator(r),
                                BusinessIdErrorCode::KeyNotFound,
                                "key unavailable"));
        }
        std::array<std::uint8_t, 32> bytes{};
        bytes.fill(7);
        return BusinessIdEncodingKey::create(r, mode == "rotated" ? 2 : 1, bytes);
    }
};

struct AllocatorProbe : BusinessIdAllocator, BusinessIdSchemaContributor {
    using allocator_type = teaql::Allocator;
    String mode;
    Vector<String>& log;
    InMemoryBusinessIdAllocator& inner;

    AllocatorProbe(std::allocator_arg_t,
                   teaql::Allocator a,
                   std::string_view mode,
                   Vector<String>& log,
                   InMemoryBusinessIdAllocator& inner)
        : mode(mode, a.resource()), log(log), inner(inner) {}

    Memory memory_resource() const {
        return mode.get_allocator().resource();
    }

    Result<BusinessIdAllocation, BusinessIdError> allocate(Memory r,
                                                           const BusinessIdPlan& p) const override {
        log.emplace_back("allocate");
        if (mode == "error") {
            return Result<BusinessIdAllocation, BusinessIdError>::err(
                r,
                BusinessIdError(std::allocator_arg,
                                teaql::Allocator(r),
                                BusinessIdErrorCode::Allocation,
                                "allocator unavailable"));
        }
        auto result = inner.allocate(r, p);
        if (result) {
            if (mode == "mismatch") {
                result.value().scope.period_key += '!';
            }
            if (mode == "outside") {
                result.value().sequence = UINT64_MAX;
            }
        }
        return result;
    }

    Result<std::monostate, BusinessIdError> ensure_schema(Memory r,
                                                          const UserContext&) const override {
        log.emplace_back("schema");
        return Result<std::monostate, BusinessIdError>::ok(r, {});
    }
};

struct Slot {
    Memory resource;
    std::optional<String> value;
    bool fresh;
    Vector<String>& log;

    std::optional<std::string_view> current_business_id() const {
        log.emplace_back("current");
        return value ? std::optional<std::string_view>(*value) : std::nullopt;
    }

    bool is_new_aggregate() const {
        log.emplace_back("fresh");
        return fresh;
    }

    void assign_business_id(BusinessIdValue v) {
        log.emplace_back("assign");
        value.emplace(v.value, resource);
    }
};

static Json evaluate(UserContext& context, const Json& j) {
    auto r = context.memory_resource();
    auto op = text(field(j, "op"));
    if (op == "allocate") {
        auto& allocator = context.make<InMemoryBusinessIdAllocator>();
        Json::Array out(r);
        for (const auto& p : std::get<Json::Array>(field(j, "plans").data)) {
            auto result = allocator.allocate(r, plan(r, p));
            if (!result) {
                out.emplace_back(error(r, result.error()));
            } else {
                auto item = detail::model_json(r);
                detail::model_put(item, "sequence", unsigned_numeric(r, result.value().sequence));
                detail::model_put(item, "scope", result.value().scope.canonical_key(r));
                out.emplace_back(std::move(item));
            }
        }
        return Json(r, out);
    }
    auto time =
        UtcDateTime::from_timestamp(number<std::int64_t>(field(j, "seconds")),
                                    std::uint32_t(std::get<std::uint64_t>(field(j, "nanos").data)));
    if (!time) {
        auto out = detail::model_json(r);
        detail::model_put(out, "valid", false);
        return out;
    }
    if (auto d = date(field(j, "override"))) {
        auto& override = context.make<BusinessDate>(*d);
        context.insert_resource(context, override);
    }
    if (op == "time") {
        auto& clock = context.make<FixedBusinessClock>(*time);
        context.set_business_clock(context, clock);
        auto out = detail::model_json(r);
        auto observed = context.business_time();
        detail::model_put(out, "valid", true);
        detail::model_put(out, "seconds", numeric(r, observed.timestamp()));
        detail::model_put(out, "nanos", observed.timestamp_subsec_nanos());
        detail::model_put(out, "date", observed.date_naive().to_string(r));
        detail::model_put(out, "businessDate", context.business_date().to_string(r));
        return out;
    }
    auto& log = context.make<Vector<String>>();
    auto& clock = context.make_resource<Clock>(*time, date(field(j, "clock_date")), log);
    context.set_business_clock(context, clock);
    auto& inner = context.make<InMemoryBusinessIdAllocator>();
    auto& allocator = context.make<AllocatorProbe>(text(field(j, "allocator")), log, inner);
    context.set_business_id_infrastructure(context, allocator);
    Json::Array out(r);
    for (const auto& step : std::get<Json::Array>(field(j, "steps").data)) {
        if (auto* s = std::get_if<String>(&field(step, "key").data); s && *s != "missing") {
            auto& provider = context.make<Key>(*s, log);
            if (!context.set_business_id_key_provider(context, provider)) {
                throw std::runtime_error("missing fixture service");
            }
        }
        Slot slot{r, {}, std::get<bool>(field(step, "new").data), log};
        if (auto* s = std::get_if<String>(&field(step, "existing").data)) {
            slot.value.emplace(*s, r);
        }
        auto result =
            context.business_ids().value().get().ensure(r,
                                                        context,
                                                        definition(r, field(step, "definition")),
                                                        text(field(step, "root")),
                                                        text(field(step, "aggregate")),
                                                        slot);
        auto observed = detail::model_json(r);
        if (result) {
            detail::model_put(observed, "ok", result.value().value);
        } else {
            observed = error(r, result.error());
        }
        auto item = detail::model_json(r);
        detail::model_put(item, "result", observed);
        detail::model_put(item, "slot", slot.value ? Json(r, *slot.value) : Json(r));
        Json::Array calls(r);
        for (const auto& call : log) {
            calls.emplace_back(call);
        }
        log.clear();
        detail::model_put(item, "calls", calls);
        out.emplace_back(std::move(item));
    }
    return Json(r, out);
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("invalid input");
        }
        std::cout << evaluate(*user, *input).dump(r) << '\n';
    }
}
