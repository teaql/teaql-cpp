#include <teaql/core/value.hpp>
#include <teaql/core/list.hpp>
#include <teaql/runtime/context.hpp>
#include <charconv>
#include <bit>
#include <iostream>
#include <limits>

using namespace teaql;
namespace {
const Json& field(const Json& j, std::string_view key) { return std::get<Json::Object>(j.data).find(key)->second; }
std::string_view text(const Json& j) { return std::get<String>(j.data); }
Json error(Memory r, std::string_view kind) { Json j(r); Json::Object o(r); o.emplace("error", Json(r, kind)); j.data = std::move(o); return j; }
template<class T> T number(std::string_view s) { T n{}; auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), n); if (ec != std::errc{} || end != s.data() + s.size()) throw std::runtime_error("Invalid fixture number"); return n; }
Json evaluate(Memory r, const Json& input) {
    auto kind = text(field(input, "kind")); const auto& data = field(input, "value"); Value v(r);
    if (kind == "text") v = Value(r, text(data));
    else if (kind == "i64") v = Value(r, number<std::int64_t>(text(data)));
    else if (kind == "u64") v = Value(r, number<std::uint64_t>(text(data)));
    else if (kind == "f64") v = Value(r, number<double>(text(data)));
    else if (kind == "f64_bits" || kind == "json_bits") {
        auto s=text(data); std::uint64_t bits=0;
        auto [end,ec]=std::from_chars(s.data(),s.data()+s.size(),bits,16);
        if(ec!=std::errc{} || end!=s.data()+s.size()) throw std::runtime_error("Invalid float bits");
        double f=std::bit_cast<double>(bits);
        v=kind=="f64_bits" ? Value(r,f) : Value(r,Json(r,f));
    }
    else if (kind == "decimal") { auto d = Decimal::parse(text(data)); if (!d) return error(r, kind); v = Value(r, *d); }
    else if (kind == "json") { auto j = Json::parse(r, text(data)); if (!j) return error(r, kind); v = Value(r, std::move(*j)); }
    else if (kind == "date") { auto d = Date::parse(text(data)); if (!d) return error(r, kind); v = Value(r, *d); }
    else if (kind == "timestamp") v = Value(r, Timestamp(number<std::int64_t>(text(data))));
    else if (kind == "bool") v = Value(r, std::get<bool>(data.data));
    else if (kind == "typed_null") v = Value(r, TypedNull{DataType::Text});
    else if (kind == "object") v = Value(r, Record(r));
    else if (kind == "list") v = Value(r, Value::List(r));
    else if (kind != "null") throw std::runtime_error("Unknown fixture kind");
    Json result(r); Json::Object o(r);
    auto format=std::get<Json::Object>(input.data).find("format");
    if(format!=std::get<Json::Object>(input.data).end() && std::get<bool>(format->second.data)) {
        o.emplace("id_key",Json(r,detail::list_id_key(r,v)));
        o.emplace("json_text",Json(r,v.to_json_value(r).dump(r)));
        result.data=std::move(o); return result;
    }
    auto put_integer = [&](const char* key, auto n) {
        Json j(r); if (n) { char b[32]; auto [end, ec] = std::to_chars(b, b + sizeof(b), *n); (void)ec; j = Json(r, std::string_view(b, end)); } o.emplace(key, std::move(j));
    };
    put_integer("i64", v.try_i64()); put_integer("u64", v.try_u64());
    auto d = v.try_decimal(); o.emplace("decimal", d ? Json(r, d->to_string(r)) : Json(r));
    auto f = v.try_f64(); o.emplace("f64", f ? Json(r, *f) : Json(r));
    auto date = v.try_date(); o.emplace("date", date ? Json(r, date->to_string(r)) : Json(r));
    auto ts = v.try_timestamp(); put_integer("timestamp", ts ? std::optional(ts->as_millis()) : std::nullopt);
    auto t = v.try_text(); o.emplace("text", t ? Json(r, *t) : Json(r));
    auto b = v.try_bool(); o.emplace("bool", b ? Json(r, *b) : Json(r));
    o.emplace("json", v.to_json_value(r)); result.data = std::move(o); return result;
}
}
int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context(); auto r = user->memory_resource();
        auto input = Json::parse(r, line); if (!input) throw std::runtime_error("Invalid fixture JSON");
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
