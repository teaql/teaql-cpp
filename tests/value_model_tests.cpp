#include <teaql/core/value.hpp>
#include <teaql/core/serde_utils.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <cmath>
#include <iostream>

using namespace teaql;

static void check(bool ok, const char* why) {
    if (!ok) {
        throw std::runtime_error(why);
    }
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

static_assert(ArenaObject<Value> && ArenaObject<Json> && ArenaObject<Decimal> &&
              ArenaObject<Date> && ArenaObject<Timestamp>);
static_assert(!std::is_copy_constructible_v<Json>);

static void json_domain(const Json& json, Memory r) {
    check(json.memory_resource() == r, "JSON value belongs to destination");
    std::visit(
        [&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, String>) {
                check(item.get_allocator().resource() == r, "JSON string domain");
            } else if constexpr (std::same_as<T, Json::Array>) {
                check(item.get_allocator().resource() == r, "JSON array domain");
                for (const auto& v : item) {
                    json_domain(v, r);
                }
            } else if constexpr (std::same_as<T, Json::Object>) {
                check(item.get_allocator().resource() == r, "JSON object domain");
                for (const auto& [key, value] : item) {
                    check(key.get_allocator().resource() == r, "JSON key domain");
                    json_domain(value, r);
                }
            }
        },
        json.data);
}

static void lifetime() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto context = global.make_user_context(64);
    auto r = context->memory_resource();
    auto g = global.memory_resource();
    auto& json = context->make<Json>();
    json = *Json::parse(
        r,
        R"({"long object key requiring dynamic allocation":{"items":["long text requiring dynamic allocation",null,true,2.5]}})");
    for (unsigned i = 0; i < 60; ++i) {
        context->make<Json>(json);
    }
    auto& value = context->make<Value>(json);
    auto& retained = global.make<Value>(value);
    auto& moved = global.make<Value>();
    moved = std::move(value); // Cross-domain move remains a deep copy.
    check(value == retained && moved == retained, "cross-domain move preserves source");
    auto& copied_json = global.make<Json>(json);
    auto& source_list = context->make<Value::List>();
    source_list.emplace_back(json);
    source_list.emplace_back(*Decimal::parse("123.4500"));
    source_list.emplace_back(*Date::parse("2024-02-29"));
    source_list.emplace_back(Timestamp(1234567));
    auto& projected = global.make<Json>(Value(r, source_list).to_json_value(g));
    auto& nested = global.make<Vector<Json>>();
    nested.emplace_back(json);
    nested.emplace_back(std::move(json));
    for (const auto& item : nested) {
        json_domain(item, g);
    }
    json_domain(copied_json, g);
    json_domain(projected, g);
    check(std::holds_alternative<Json>(retained.data) && !retained.as_object(),
          "JSON remains distinct from Value Object");
    check(context->allocation_stats().total_blocks > 1, "JSON causes arena growth");
    context->close();
    check(context->allocation_stats().live_blocks == 0 &&
              context->allocation_stats().live_bytes == 0,
          "all user blocks reclaimed");
    json_domain(std::get<Json>(retained.data), g);
    check(retained == moved && copied_json == std::get<Json>(retained.data),
          "deep copies survive source close");
    check(projected.dump(g).find("123.4500") != String::npos,
          "decimal scale survives JSON projection");
}

static void scalar_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto max = Decimal::parse("79228162514264337593543950335");
    check(max && !max->to_i64() && !max->to_u64(), "96-bit range exceeds integer primitives");
    check(!Decimal::parse("79228162514264337593543950336"), "coefficient overflow");
    check(Decimal::parse("1.00") == Decimal::parse("1"), "numeric decimal equality ignores scale");
    check(Decimal::parse("-0.00") == Decimal::parse("0"), "signed decimal zero equality");
    check(!Decimal::parse("-0.1")->to_u64() && Decimal::parse("-0.1")->to_i64() == 0,
          "unsigned negative fraction rejects before truncation");
    check(Decimal::from_i64(INT64_MIN).to_i64() == INT64_MIN, "minimum signed round trip");
    check(Decimal::from_u64(UINT64_MAX).to_u64() == UINT64_MAX, "maximum unsigned round trip");
    auto zero = Decimal::from_f64(-0.0);
    check(zero && zero->is_sign_negative() && !zero->to_u64(),
          "float negative zero retains decimal sign");
    check(!Decimal::from_parts({0, 0, 0}, false, 29), "invalid scale rejects");
    auto date = *Date::parse("2024-02-29");
    auto timestamp = Timestamp(1709164800000);
    check(!Value(r, date).try_timestamp() && !Value(r, timestamp).try_date(),
          "Rust conversion distinction between Date and Timestamp");
    check(!Value(r, 123.0).try_i64() && !Value(r, "123").try_i64(),
          "no implicit float/text integer conversion");
    check(Value(r, -1).try_date()->to_string(r) == "1969-12-31",
          "negative milliseconds floor to previous UTC day");
    check(Timestamp(-1).to_datetime() == DateTime{*Date::parse("1969-12-31"), 23, 59, 59, 999},
          "negative timestamp components");
    check(Timestamp(INT64_MAX).to_datetime() == DateTime{} &&
              Timestamp(INT64_MIN).to_datetime() == DateTime{},
          "out-of-range datetime defaults to epoch");
    check(Timestamp::from_u64(UINT64_MAX).as_millis() == -1 &&
              !Value(r, UINT64_MAX).try_timestamp(),
          "wrapper conversion wraps; Value conversion checks");
    for (auto text : {"-262143-01-01", "+262142-12-31", "0000-02-29"}) {
        auto d = *Date::parse(text);
        check(Date::from_epoch_days(d.epoch_days()) == d, "full chrono date range round trip");
    }
    check(!Date::from_ymd(2023, 2, 29) && !Date::from_ymd(262143, 1, 1), "date limits");
}

static void json_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    for (auto bad : {"[",
                     "{",
                     "[1,]",
                     "{\"a\":1,}",
                     "01",
                     "+1",
                     "1.",
                     "-.1",
                     "1e+",
                     "\"\\ud800\"",
                     "\"\\udc00\"",
                     "\"\\ud800\\u0000\"",
                     "1e999",
                     "\"\xc0\xaf\"",
                     "\"\xf4\x90\x80\x80\""}) {
        check(!Json::parse(r, bad), "invalid JSON rejects");
    }
    auto& valid =
        global.make<Json>(*Json::parse(r, R"({"a":"\u0000\ud83d\ude80雪","same":1,"same":2})"));
    check(Json::parse(r, valid.dump(r)) == std::optional<Json>(Json(r, valid)),
          "JSON Unicode and escapes round trip");
    check(std::get<std::uint64_t>(std::get<Json::Object>(valid.data).at(String("same", r)).data) ==
              2,
          "duplicate key last value wins");
    check(*Json::parse(r, "1") != *Json::parse(r, "1.0"), "JSON number integer/float distinction");
    check(std::signbit(std::get<double>(Json::parse(r, "-0")->data)),
          "JSON negative zero is float");
    String depth(127, '[', r);
    depth.append(127, ']');
    check(bool(Json::parse(r, depth)), "serde recursion depth accepts 127 containers");
    depth.insert(depth.begin(), '[');
    depth += ']';
    check(!Json::parse(r, depth), "serde recursion depth rejects 128 containers");
}

struct FailingResource : std::pmr::memory_resource {
    bool fail = false;
    std::size_t live = 0;

    void* do_allocate(std::size_t size, std::size_t alignment) override {
        if (fail) {
            throw std::bad_alloc();
        }
        ++live;
        return std::pmr::new_delete_resource()->allocate(size, alignment);
    }

    void do_deallocate(void* p, std::size_t size, std::size_t alignment) override {
        --live;
        std::pmr::new_delete_resource()->deallocate(p, size, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& rhs) const noexcept override {
        return this == &rhs;
    }
};

static void clone_failure() {
    NoDefaultAllocator guard;
    FailingResource upstream;
    {
        GlobalContext source;
        auto r = source.memory_resource();
        GlobalContext destination(&upstream, 64);
        auto& large = source.make<Value>(String(100000, 'x', r));
        auto& target = destination.make<Value>(42);
        upstream.fail = true;
        bool failed = false;
        try {
            target = std::move(large);
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        check(failed && target.try_i64() == 42 && large.try_text()->size() == 100000,
              "Value assignment failure preserves both domains");
        upstream.fail = false;
        auto& json = source.make<Json>(String(100000, 'y', r));
        auto& other = destination.make<Json>(true);
        upstream.fail = true;
        failed = false;
        try {
            other = std::move(json);
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        check(failed && std::get<bool>(other.data) && std::get<String>(json.data).size() == 100000,
              "JSON clone failure preserves both domains");
    }
    check(upstream.live == 0, "failed clones reclaim destination blocks");
}

static void serialization_helpers() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    using namespace serde_utils;
    using OptionalStringResult = Result<std::optional<String>, serde_utils::Error>;
    OptionalStringResult* retained = nullptr;
    {
        Json text(r, "\xe3\x80\x80  long string requiring allocation  \xc2\xa0");
        auto required = trimmed_string::deserialize(r, text);
        check(required && required.value() == "long string requiring allocation",
              "required deserialization trims Unicode whitespace");
        auto optional = trimmed_opt_string::deserialize(r, text);
        check(optional && optional.value() && *optional.value() == required.value(),
              "optional string deserialization");
        retained = &global.make<OptionalStringResult>(optional);
        auto whitespace = trimmed_opt_string::deserialize(r, Json(r, " \t\n\xe3\x80\x80"));
        check(whitespace && whitespace.value() && whitespace.value()->empty(),
              "optional whitespace is Some(empty), not None");
        auto absent = trimmed_opt_string::deserialize(r, Json(r));
        check(absent && !absent.value(), "JSON null is optional None");
        check(!trimmed_string::deserialize(r, Json(r)) &&
                  !trimmed_string::deserialize(r, Json(r, 1)) &&
                  !trimmed_opt_string::deserialize(r, Json(r, true)),
              "wrong JSON kinds return typed deserialization errors");
        check(trimmed_string::serialize(r, "\xc2\xa0 hello \t").dump(r) == "\"hello\"",
              "serialization trims without changing stored data");
        check(trimmed_opt_string::serialize(r, std::nullopt).dump(r) == "null" &&
                  trimmed_opt_string::serialize(r, std::string_view(" \n ")).dump(r) == "\"\"",
              "optional serialization keeps None distinct from empty");
        for (auto n : {INT64_MIN, std::int64_t(-1), std::int64_t(0), INT64_MAX}) {
            auto json = Timestamp(n).to_json_value(r);
            auto decoded = Timestamp::from_json_value(json);
            check(decoded && decoded->as_millis() == n, "Timestamp transparent integer round trip");
        }
        check(!Timestamp::from_json_value(Json(r, UINT64_MAX)) &&
                  !Timestamp::from_json_value(Json(r, 1.0)) &&
                  !Timestamp::from_json_value(Json(r, "1")) && !Timestamp::from_json_value(Json(r)),
              "Timestamp serde rejects unsigned overflow, float, string and null");
    }
    user->close();
    check(retained->value()->get_allocator().resource() == global.memory_resource() &&
              *retained->value() == "long string requiring allocation",
          "optional decoded text deep copy survives source close");
}

int main() {
    try {
        lifetime();
        scalar_contracts();
        json_contracts();
        clone_failure();
        serialization_helpers();
        std::cout << "Value models and lifetime contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
