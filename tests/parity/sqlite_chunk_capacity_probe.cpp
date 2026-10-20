#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <charconv>
#include <iostream>
#include <sqlite3.h>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static CompiledQuery query(Memory resource) {
    CompiledQuery result(std::allocator_arg, Allocator(resource));
    result.sql = "SELECT id FROM stream_probe ORDER BY id";
    result.comment.emplace(
        "what: observe bounded cursor allocation; why: avoid chunk-buffer growth", resource);
    return result;
}

struct Fixture {
    GlobalContext& global;
    SqliteConnection& connection;
    SqliteMutationExecutor& provider;
    std::unique_ptr<UserContext> user_context;
    Memory memory;

    Fixture(GlobalContext& global, std::size_t rows)
        : global(global), connection(global.make_resource<SqliteConnection>()),
          provider(global.make<SqliteMutationExecutor>(connection)),
          user_context(global.make_user_context()), memory(user_context->memory_resource()) {
        CompiledQuery setup(std::allocator_arg, Allocator(memory));
        setup.sql = "CREATE TABLE stream_probe(id INTEGER)";
        check(bool(provider.execute(memory, setup)), "Create original cursor fixture");
        char digits[32];
        auto end = std::to_chars(digits, digits + sizeof(digits), rows).ptr;
        String number(digits, end, memory);
        setup.sql =
            "INSERT INTO stream_probe SELECT x FROM (WITH RECURSIVE seq(x) AS (SELECT 1 WHERE ";
        setup.sql += number;
        setup.sql += " > 0 UNION ALL SELECT x+1 FROM seq WHERE x < ";
        setup.sql += number;
        setup.sql += ") SELECT x FROM seq)";
        check(bool(provider.execute(memory, setup)), "Seed original recursive cursor fixture");
    }
};

static std::size_t drain(QueryStream<MutationExecutorError>& stream, std::size_t size) {
    std::size_t count = 0;
    while (const auto* item = stream.wait_next()) {
        check(bool(*item), "Cursor chunk succeeds");
        const auto& chunk = item->value();
        check(size == 0 || chunk.rows.size() <= size, "Original chunk row bound");
        for (const auto& row : chunk.rows) {
            const auto* id = row.get("id");
            check(id && std::get<std::int64_t>(id->data) == std::int64_t(++count),
                  "Every original cursor row has its ordered I64 ID");
        }
    }
    return count;
}

static const Json& field(const Json& input, std::string_view name) {
    return std::get<Json::Object>(input.data).find(name)->second;
}

static Json observe(GlobalContext& global, const Json& input) {
    auto destination = global.memory_resource();
    const auto count = std::get<std::uint64_t>(field(input, "rows").data);
    const auto size = std::get<std::uint64_t>(field(input, "chunk_size").data);
    const auto& consume = field(input, "consume");
    const bool unlimited = std::holds_alternative<std::monostate>(consume.data);
    const auto limit = unlimited ? std::uint64_t(0) : std::get<std::uint64_t>(consume.data);
    Fixture fixture(global, count);
    SqlTransportTransaction<MutationExecutorError>* transaction = nullptr;
    if (std::get<bool>(field(input, "transaction").data)) {
        transaction = &fixture.provider.begin_sql(fixture.memory).wait().value().get();
    }
    StreamingSqlTransport<MutationExecutorError>* transport = &fixture.provider;
    if (transaction) {
        transport = dynamic_cast<StreamingSqlTransport<MutationExecutorError>*>(transaction);
        check(transport, "Original transaction supports native streaming");
    }
    auto& stream = transport->stream_sql(fixture.memory, query(fixture.memory), size);
    Json::Array chunks(destination);
    bool exhausted = false;
    while (unlimited || chunks.size() < limit) {
        const auto* item = stream.wait_next();
        if (!item) {
            exhausted = true;
            break;
        }
        check(bool(*item), "Observed native chunk succeeds");
        const auto& chunk = item->value();
        Json::Array ids(destination);
        for (const auto& row : chunk.rows) {
            const auto* id = row.get("id");
            check(id, "Observed cursor ID is present");
            ids.emplace_back(id->to_json_value(destination));
        }
        auto entry = detail::model_json(destination);
        detail::model_put(entry, "capacity", chunk.rows.capacity());
        detail::model_put(entry, "ids", ids);
        detail::model_put(entry, "chunk_index", chunk.chunk_index);
        detail::model_put(entry, "is_last", chunk.is_last);
        chunks.emplace_back(entry);
    }
    stream.cancel();
    if (transaction) {
        check(bool(transaction->rollback_sql(fixture.memory).wait()),
              "Release original transaction after cursor close");
    }
    const auto& again =
        fixture.provider.fetch_all_compact_sql(fixture.memory, query(fixture.memory)).wait();
    check(again && again.value().size() == count,
          "Original cursor close permits complete provider reuse");
    std::size_t index = 0;
    for (const auto& row : again.value()) {
        const auto* id = row.get("id");
        check(id && std::get<std::int64_t>(id->data) == std::int64_t(++index),
              "All reuse IDs retain original order");
    }
    auto output = detail::model_json(destination);
    detail::model_put(output, "chunks", chunks);
    detail::model_put(output, "exhausted", exhausted);
    detail::model_put(output, "reuse_rows", again.value().size());
    detail::model_put(
        output, "autocommit", bool(sqlite3_get_autocommit(fixture.connection.native_handle())));
    return Json(destination, output);
}

class CountingUpstream final : public std::pmr::memory_resource {
public:
    std::size_t calls = 0, bytes = 0, live = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        auto* value = std::pmr::new_delete_resource()->allocate(size, alignment);
        ++calls;
        bytes += size;
        live += size;
        return value;
    }

    void do_deallocate(void* value, std::size_t size, std::size_t alignment) override {
        live -= size;
        std::pmr::new_delete_resource()->deallocate(value, size, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void report_real_cursor_chunk_allocations() {
    for (const auto [rows, size] : {std::pair<std::size_t, std::size_t>(0, 73),
                                    {1, 73},
                                    {73, 73},
                                    {74, 73},
                                    {219, 73},
                                    {223, 73},
                                    {4096, 128}}) {
        CountingUpstream upstream;
        {
            GlobalContext global(&upstream);
            Fixture fixture(global, rows);
            check(drain(fixture.provider.stream_sql(fixture.memory, query(fixture.memory), size),
                        size) == rows,
                  "Original warm drain returns every row");
            const auto before_calls = upstream.calls;
            const auto before_bytes = upstream.bytes;
            check(drain(fixture.provider.stream_sql(fixture.memory, query(fixture.memory), size),
                        size) == rows,
                  "Original measured drain returns every row");
            // These are C++ arena backing blocks, not Rust process allocation counts.
            std::cout << "CPP_STREAM_ARENA_BLOCKS," << rows << ',' << size << ','
                      << upstream.calls - before_calls << ',' << upstream.bytes - before_bytes
                      << '\n';
        }
        check(upstream.live == 0, "All fixture Context backing blocks return after destruction");
    }
}

static void every_chunk_buffer_has_bounded_capacity_and_releases_on_drop() {
    GlobalContext global;
    Fixture fixture(global, 223);
    auto& stream = fixture.provider.stream_sql(fixture.memory, query(fixture.memory), 73);
    std::size_t index = 0;
    while (const auto* item = stream.wait_next()) {
        check(bool(*item), "Original bounded native chunk succeeds");
        const auto& chunk = item->value();
        check(chunk.rows.capacity() <= 73, "Chunk must not geometrically overallocate beyond 73");
        check(chunk.rows.capacity() == (chunk.rows.size() == 73 ? 73 : 4),
              "Original full/tail capacities are exactly 73/4");
        check(chunk.chunk_index == index++, "Original sequential chunk index");
    }
    check(index == 4, "Original 223-row cursor has four chunks");
    auto& early = fixture.provider.stream_sql(fixture.memory, query(fixture.memory), 73);
    const auto* item = early.wait_next();
    check(item && *item && item->value().rows.size() == 73,
          "Original early cursor exposes 73 rows");
    early.cancel();
    check(drain(fixture.provider.stream_sql(fixture.memory, query(fixture.memory), 73), 73) == 223,
          "Original early-drop adaptation releases cursor and lease before reuse");
}

static void zero_chunk_size_preserves_the_existing_single_tail_behavior() {
    GlobalContext global;
    Fixture fixture(global, 3);
    auto& stream = fixture.provider.stream_sql(fixture.memory, query(fixture.memory), 0);
    const auto* item = stream.wait_next();
    check(item && *item && item->value().rows.size() == 3 && item->value().is_last,
          "Original zero-sized native chunk is one three-row terminal tail");
    check(stream.wait_next() == nullptr, "Original zero-sized cursor reaches EOF after its tail");
}

static void actual_context_deletion_releases_capacity_and_cursor() {
    CountingUpstream upstream;
    {
        GlobalContext global(&upstream);
        Fixture fixture(global, 223);
        auto& stream = fixture.provider.stream_sql(fixture.memory, query(fixture.memory), 73);
        const auto* item = stream.wait_next();
        check(item && *item && item->value().rows.size() == 73,
              "Open capacity-bounded cursor before Context deletion");
        auto next_user_context = global.make_user_context();
        auto next_memory = next_user_context->memory_resource();
        auto& waiting = fixture.provider.begin_sql(next_memory);
        check(waiting.poll() == nullptr, "Another transaction waits for the cursor lease");
        const auto before = global.live_user_bytes();
        fixture.user_context.reset();
        check(global.live_user_bytes() < before,
              "Actual UserContext deletion returns its row and chunk blocks");
        auto& transaction = waiting.wait().value().get();
        check(bool(transaction.rollback_sql(next_memory).wait()),
              "Waiting transaction resumes after cursor owner deletion");
        check(drain(fixture.provider.stream_sql(next_memory, query(next_memory), 73), 73) == 223,
              "Provider and original ordered data remain usable through another Context");
    }
    check(upstream.live == 0, "Every capacity/lifetime fixture block returns to its upstream");
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--contracts") {
            report_real_cursor_chunk_allocations();
            every_chunk_buffer_has_bounded_capacity_and_releases_on_drop();
            zero_chunk_size_preserves_the_existing_single_tail_behavior();
            actual_context_deletion_releases_capacity_and_cursor();
            std::cout
                << "Original three cursor allocation contracts and actual Context deletion passed\n";
            return 0;
        }
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            check(bool(input), "Valid capacity fixture JSON");
            const auto output = observe(global, *input);
            check(global.live_user_bytes() == 0,
                  "Observed UserContext blocks return before retained Global JSON use");
            std::cout << output.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
