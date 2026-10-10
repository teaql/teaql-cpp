#include <teaql/runtime.hpp>
#include <teaql/runtime/query_store_debug.hpp>
#include <charconv>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static std::int64_t integer_field(std::string_view text, std::string_view name) {
    auto begin = text.find(name);
    check(begin != text.npos, "Instant diagnostics retain their source field");
    text.remove_prefix(begin + name.size());
    std::int64_t result{};
    auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    check(parsed.ec == std::errc{}, "Instant fields are numeric observations");
    return result;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto cursor = user->make_ref<ContinuousPageCursor>();
    auto& input = user->get(cursor);
    input.cursor_id.assign(512, 'c');
    input.query_key.assign(512, 'k');
    input.entity = "Item";
    input.boundary = Value(owner, std::string_view("nested boundary"));
    input.expires_at = SystemTime::from_unix_seconds(253402300800, 123456789);
    auto kept_cursor = user->make_global<ContinuousPageCursor>(input);
    auto& ids = user->make<RetainedIdSet>();
    ids.query_key.assign(512, 'q');
    ids.ids = {0, 1, UINT64_MAX};
    ids.expires_at = input.expires_at;
    auto kept_ids = user->make_global<RetainedIdSet>(ids);
    auto kept_cache = user->make_global<InMemoryAggregationCache>("ns");
    {
        Record fields(owner);
        fields.emplace(String("value", owner), Value(owner, "retained cache payload"));
        auto& row = user->make<CompactRow>(fields);
        auto before = std::chrono::steady_clock::now();
        auto& cache = user->get(kept_cache);
        cache.put("ns::item", std::span<const CompactRow>(&row, 1));
        auto after = std::chrono::steady_clock::now();
        auto diagnostic = debug_string(owner, cache);
        auto seconds = integer_field(diagnostic, "tv_sec: ");
        auto nanos = integer_field(diagnostic, "tv_nsec: ");
        check(nanos >= 0 && nanos < 1000000000, "Instant nanoseconds remain normalized");
        auto observed = std::chrono::seconds(seconds) + std::chrono::nanoseconds(nanos);
        check(observed >= before.time_since_epoch() && observed <= after.time_since_epoch(),
              "Populated cache diagnostics retain the actual monotonic timestamp");
        check(diagnostic.find("AggregationCacheEntry") != diagnostic.npos &&
                  diagnostic.find("retained cache payload") != diagnostic.npos,
              "Populated cache diagnostics include the actual rows");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "Source User is fully reclaimed");
    auto next = global.make_user_context();
    const auto& retained = next->get(next->rebind_global(kept_cursor));
    const auto& retained_ids = next->get(next->rebind_global(kept_ids));
    check(retained.query_key.get_allocator().resource() == global.memory_resource() &&
              retained.boundary.memory_resource() == global.memory_resource() &&
              retained_ids.ids.get_allocator().resource() == global.memory_resource(),
          "Nested strings, values and ID storage move to the destination scope");
    {
        auto rows =
            next->get(next->rebind_global(kept_cache)).get(next->memory_resource(), "ns::item", 0);
        check(rows && rows->size() == 1 && rows->front().get("value")->try_text() &&
                  *rows->front().get("value")->try_text() == "retained cache payload",
              "Global cache owns copied rows after actual source User deletion");
        auto text = debug_string(next->memory_resource(), retained, DebugOptions{.pretty = true});
        check(text.get_allocator().resource() == next->memory_resource() &&
                  text.find("nested boundary") != text.npos,
              "Retained cursor diagnostics allocate in the caller's current User");
    }
    bool rejected = false;
    try {
        (void)next->get(cursor);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired cursor access is rejected before formatting or copying");
    std::cout << "Query-store model ownership and monotonic-clock contracts pass\n";
}
