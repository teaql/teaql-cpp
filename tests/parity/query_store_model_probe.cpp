#include <teaql/runtime.hpp>
#include <teaql/runtime/query_store_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <iostream>

static SystemTime expiry(const Json& input) {
    return SystemTime::from_unix_seconds(
        number<std::int64_t>(field(input, "seconds")),
        detail::model_integer<std::uint32_t>(field(input, "nanos"), "nanos"));
}

static ContinuousPageCursor cursor(Memory owner, const Json& input) {
    ContinuousPageCursor result(std::allocator_arg, Allocator(owner));
    result.cursor_id = text(field(input, "cursor"));
    result.query_key = text(field(input, "key"));
    result.entity = text(field(input, "entity"));
    result.direction =
        text(field(input, "direction")) == "Asc" ? SortDirection::Asc : SortDirection::Desc;
    result.boundary = value(owner, field(input, "boundary"));
    result.page_size = number<std::uint64_t>(field(input, "page_size"));
    result.next_offset = number<std::uint64_t>(field(input, "offset"));
    result.expires_at = expiry(input);
    return result;
}

static std::int64_t instant_number(std::string_view input, std::string_view key) {
    auto start = input.find(key);
    if (start == input.npos) {
        throw std::runtime_error("Missing Instant diagnostic field");
    }
    input.remove_prefix(start + key.size());
    std::int64_t result{};
    auto parsed = std::from_chars(input.data(), input.data() + input.size(), result);
    if (parsed.ec != std::errc{}) {
        throw std::runtime_error("Invalid Instant diagnostic field");
    }
    return result;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto output = detail::model_json(owner);
    auto put = [&](std::string_view name, auto&& value) {
        detail::model_put(output, name, std::forward<decltype(value)>(value));
    };
    auto kind = text(field(input, "kind"));
    if (kind == "cursor") {
        auto value = cursor(owner, input);
        auto clone = detail::clone_in(owner, value);
        auto other = cursor(owner, field(input, "other"));
        put("debug", formatted_debug(owner, value));
        put("clone", formatted_debug(owner, clone));
        put("clone_equal", value == clone);
        put("other_equal", value == other);
    } else if (kind == "ids") {
        RetainedIdSet value(std::allocator_arg, Allocator(owner));
        value.query_key = text(field(input, "key"));
        value.expires_at = expiry(input);
        for (const auto& id : detail::model_array(field(input, "ids"), "ids")) {
            value.ids.push_back(number<std::uint64_t>(id));
        }
        auto clone = detail::clone_in(owner, value);
        put("debug", formatted_debug(owner, value));
        put("clone", formatted_debug(owner, clone));
    } else if (kind == "cache_live") {
        auto& cache = user.make<InMemoryAggregationCache>("ns");
        Record fields(owner);
        fields.emplace(String("value", owner), Value(owner, "clock payload"));
        CompactRow row(std::allocator_arg, Allocator(owner), fields);
        auto row_debug = debug_string(owner, row);
        auto before = std::chrono::steady_clock::now();
        cache.put("ns::item", std::span<const CompactRow>(&row, 1));
        auto after = std::chrono::steady_clock::now();
        auto debug = debug_string(owner, cache);
        auto ticks = std::chrono::seconds(instant_number(debug, "tv_sec: ")) +
                     std::chrono::nanoseconds(instant_number(debug, "tv_nsec: "));
        auto rows = cache.get(owner, "ns::item", 0);
        put("within_put_bracket",
            ticks >= before.time_since_epoch() && ticks <= after.time_since_epoch());
        put("row_debug_included", debug.find(row_debug) != debug.npos);
        put("read_preserves_timestamp", debug == debug_string(owner, cache));
        put("rows_equal",
            rows && rows->size() == 1 && debug_string(owner, rows->front()) == row_debug);
    } else if (kind == "cache") {
        auto& empty = user.make<InMemoryAggregationCache>();
        auto& named = user.make<InMemoryAggregationCache>(text(field(input, "namespace")));
        put("default", formatted_debug(owner, empty));
        put("default_namespace", empty.name_space());
        put("named", formatted_debug(owner, named));
        put("namespace", named.name_space());
    } else {
        throw std::invalid_argument("Unknown query-store model fixture");
    }
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid query-store fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
