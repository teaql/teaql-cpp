#include <teaql/runtime/context.hpp>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

// Tracks real upstream calls, including exact size/alignment returned on deallocation.
class TrackingResource final : public std::pmr::memory_resource {
public:
    std::map<void*, std::pair<std::size_t, std::size_t>> allocations;
    std::size_t total = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* ptr = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        allocations.emplace(ptr, std::make_pair(bytes, alignment));
        ++total;
        return ptr;
    }

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
        auto it = allocations.find(ptr);
        if (it == allocations.end() || it->second != std::make_pair(bytes, alignment)) {
            std::terminate();
        }
        allocations.erase(it);
        std::pmr::new_delete_resource()->deallocate(ptr, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

struct Resource {
    int id;
    std::vector<int>& order;
    TrackingResource& upstream;

    Resource(int id, std::vector<int>& order, TrackingResource& upstream)
        : id(id), order(order), upstream(upstream) {}

    ~Resource() noexcept {
        if (upstream.allocations.empty()) {
            std::terminate(); // Destruction precedes arena release.
        }
        order.push_back(id);
    }
};

struct Throws {
    explicit Throws(int& destructed) : destructed(destructed) {
        throw std::runtime_error("constructor failure");
    }

    ~Throws() noexcept {
        ++destructed;
    }

    int& destructed;
};

struct alignas(256) Aligned {
    std::array<std::byte, 256> data;
};

static void growth_and_cleanup() {
    TrackingResource upstream;
    std::vector<int> order;
    order.reserve(3);
    GlobalContext global(&upstream);
    UserContext context(global, 128);
    auto& text = context.make<std::pmr::string>(2048, 'x');
    auto& rows = context.make<std::pmr::vector<std::pmr::string>>();
    for (int i = 0; i < 100; ++i) {
        rows.emplace_back(128, 'a');
    }
    check(text.get_allocator().resource() == context.memory_resource(),
          "string uses context allocator");
    check(rows.get_allocator().resource() == context.memory_resource(),
          "vector uses context allocator");
    check(rows.front().get_allocator().resource() == context.memory_resource(),
          "nested allocator propagation");
    auto& aligned = context.make<Aligned>();
    check(reinterpret_cast<std::uintptr_t>(&aligned) % 256 == 0, "over-aligned object");
    context.make_resource<Resource>(1, order, upstream);
    context.make_resource<Resource>(2, order, upstream);
    auto stats = context.allocation_stats();
    check(stats.live_blocks > 1 && stats.live_bytes > 0, "arena grows across blocks");
    check(stats.live_blocks + global.allocation_stats().live_blocks == upstream.allocations.size(),
          "User blocks and Global control metadata accounted separately");
    context.close();
    check(order == std::vector<int>({2, 1}), "reverse resource cleanup");
    check(upstream.allocations.size() == global.allocation_stats().live_blocks,
          "every User block returned; only Global control metadata remains");
    check(context.allocation_stats().live_bytes == 0 && context.allocation_stats().live_blocks == 0,
          "zero live context allocations");
    context.close();
    check(order.size() == 2, "close is idempotent");
    bool rejected = false;
    try {
        context.make<int>(1);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected, "closed context cannot allocate objects");
    rejected = false;
    try {
        (void)context.memory_resource()->allocate(1);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected, "closed allocator cannot allocate directly");
}

static void exception_and_isolation() {
    TrackingResource upstream;
    GlobalContext global(&upstream);
    int destructed = 0;
    {
        UserContext outer(global, 64);
        auto& retained = outer.make<std::pmr::string>(1000, 'o');
        auto outer_blocks = upstream.allocations.size();
        try {
            UserContext inner(global, 64);
            inner.make<std::pmr::vector<int>>(10000, 7);
            inner.make_resource<Throws>(destructed);
            throw std::logic_error("unreachable");
        } catch (const std::runtime_error&) {
        }
        check(destructed == 0, "unconstructed object is not destroyed");
        check(upstream.allocations.size() == outer_blocks, "exception releases inner context only");
        check(retained.size() == 1000 && retained.front() == 'o', "other context survives");
    }
    check(upstream.allocations.size() == global.allocation_stats().live_blocks,
          "scope destruction returns all User blocks");
    for (int n = 0; n < 1000; ++n) {
        UserContext context(global, 64);
        context.make<std::pmr::vector<int>>(1000, n);
    }
    check(upstream.allocations.size() == global.allocation_stats().live_blocks,
          "repeated Context destruction retains only bounded Global identity records");
    bool failed = false;
    try {
        GlobalContext failing_global(std::pmr::null_memory_resource());
        UserContext context(failing_global, 64);
        context.make<int>(1);
    } catch (const std::bad_alloc&) {
        failed = true;
    }
    check(failed, "allocation failure propagates");
}

static_assert(!std::is_default_constructible_v<UserContext>);
static_assert(!ArenaObject<std::string>);
static_assert(!ArenaObject<std::vector<int>>);
static_assert(ArenaObject<std::pmr::string>);
static_assert(ArenaObject<std::pmr::vector<int>>);

static void explicit_domains() {
    TrackingResource upstream;
    {
        GlobalContext global(&upstream);
        auto& metadata = global.make<std::pmr::string>(2048, 'g');
        auto global_blocks = global.allocation_stats().live_blocks;
        auto context = global.make_user_context(128);
        auto& rows = context->make<std::pmr::vector<std::pmr::string>>();
        rows.emplace_back(2000, 'c');
        check(metadata.get_allocator().resource() == global.memory_resource(),
              "global allocator explicit");
        check(rows.front().get_allocator().resource() == context->memory_resource(),
              "context allocator explicit");
        check(global.memory_resource() != context->memory_resource(),
              "distinct allocation domains");
        // Explicit deep copy into the destination domain, including nested strings.
        auto& retained = global.make<std::pmr::vector<std::pmr::string>>(rows);
        check(retained.front().get_allocator().resource() == global.memory_resource(),
              "copy changes ownership recursively");
        context.reset();
        check(retained.front().size() == 2000 && retained.front().front() == 'c',
              "explicit global copy survives");
        check(metadata.size() == 2048, "global metadata survives context destruction");
        check(global.allocation_stats().live_blocks >= global_blocks, "global blocks remain owned");
        check(upstream.allocations.size() == global.allocation_stats().live_blocks,
              "context blocks never retained in global arena");
    }
    check(upstream.allocations.empty(), "global destruction releases global blocks");
}

int main() {
    try {
        growth_and_cleanup();
        exception_and_isolation();
        explicit_domains();
        std::cout << "Context lifetime contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
