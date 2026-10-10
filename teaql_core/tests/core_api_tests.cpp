#include <teaql/runtime/context.hpp>
#include <concepts>
#include <iostream>
#include <map>
#include <unordered_map>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(std::totally_ordered<Timestamp>);
static_assert(Timestamp(INT64_MIN) < Timestamp(0));
static_assert(Timestamp(INT64_MAX) > Timestamp(-1));
static_assert(Timestamp{}.as_millis() == 0);
static_assert(noexcept(std::hash<Timestamp>{}(Timestamp{})));

static void timestamp_container_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& ordered = user->make<std::pmr::map<Timestamp, unsigned>>();
    auto& hashed = user->make<std::pmr::unordered_map<Timestamp, unsigned>>();
    const std::int64_t values[] = {INT64_MAX, -1, 0, INT64_MIN, 1, -1, 0};
    for (auto value : values) {
        ++ordered[Timestamp(value)];
        ++hashed[Timestamp(value)];
    }
    check(ordered.size() == 5 && hashed.size() == 5 &&
              ordered.begin()->first == Timestamp(INT64_MIN) &&
              ordered.rbegin()->first == Timestamp(INT64_MAX),
          "signed timestamp order in standard containers");
    auto wrapped = Timestamp::from_u64(UINT64_MAX);
    check(wrapped == Timestamp(-1) && hashed.at(wrapped) == 2 &&
              std::hash<Timestamp>{}(wrapped) == std::hash<Timestamp>{}(Timestamp(-1)),
          "equal timestamps from distinct constructors hash consistently");
    for (const auto& [timestamp, count] : ordered) {
        check(hashed.at(timestamp) == count,
              "ordered and hashed lookup retain all signed boundaries");
    }
}

static void dynamic_mask_lifetimes() {
    GlobalContext global;
    Ref<EntityDescriptor> retained;
    {
        auto user = global.make_user_context();
        auto& input = user->make<Vector<String>>();
        input.emplace_back(4096, 'a');
        input.emplace_back(4096, 'b');
        input.emplace_back(input[0]);
        input.emplace_back(4096, 'c');
        retained = user->make_global<EntityDescriptor>("Customer");
        auto& descriptor = user->get(retained);
        descriptor.mask_fields(std::span<const String>(input));
        check(descriptor.audit_mask_fields_declared && descriptor.audit_mask_fields.size() == 4 &&
                  descriptor.audit_mask_fields[0] == input[0] &&
                  descriptor.audit_mask_fields[2] == input[0],
              "runtime mask input preserves order and duplicates");
        for (const auto& field : descriptor.audit_mask_fields) {
            check(field.get_allocator().resource() == global.memory_resource(),
                  "mask character storage belongs to destination");
        }
        input[0].assign(4096, 'z');
        check(descriptor.audit_mask_fields[0].front() == 'a',
              "input mutation does not change Global fields");
        descriptor.mask_fields(descriptor.audit_mask_fields | std::views::reverse);
        check(descriptor.audit_mask_fields[0].front() == 'c' &&
                  descriptor.audit_mask_fields[1].front() == 'a' &&
                  descriptor.audit_mask_fields[2].front() == 'b',
              "self-aliasing reverse view copies before replacement");
        descriptor.mask_fields(std::span<const String>(descriptor.audit_mask_fields));
        check(descriptor.audit_mask_fields[2].size() == 4096,
              "self-aliasing span remains complete");
        auto& empty = user->make<EntityDescriptor>("Legacy");
        check(!empty.audit_mask_fields_declared, "legacy unspecified policy");
        Vector<String> no_names(user->memory_resource());
        empty.mask_fields(no_names);
        check(empty.audit_mask_fields_declared && empty.audit_mask_fields.empty(),
              "dynamic empty policy is explicitly declared");
        std::vector<std::string> native_names{"name", "password", "name"};
        empty.mask_fields(native_names);
        check(empty.audit_mask_fields.size() == 3 && empty.audit_mask_fields[1] == "password" &&
                  empty.audit_mask_fields[1].get_allocator().resource() == user->memory_resource(),
              "native input ranges still copy into selected Context");
    }
    auto next = global.make_user_context();
    auto& descriptor = next->get(next->rebind_global(retained));
    check(descriptor.audit_mask_fields_declared &&
              descriptor.audit_mask_fields[0] == String(4096, 'c', next->memory_resource()) &&
              descriptor.audit_mask_fields[2].size() == 4096 &&
              descriptor.audit_mask_fields[2].front() == 'b',
          "complete Global mask fields survive actual User deletion");
}

class FailingUpstream final : public std::pmr::memory_resource {
public:
    std::optional<std::size_t> fail_at;
    std::size_t attempts = 0, retained_bytes = 0, retained_blocks = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto attempt = attempts++;
        if (fail_at == attempt) {
            throw std::bad_alloc{};
        }
        auto* result = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        retained_bytes += bytes;
        ++retained_blocks;
        return result;
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
        retained_bytes -= bytes;
        --retained_blocks;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void mask_allocation_failures() {
    for (bool declared : {false, true}) {
        unsigned failures = 0;
        bool completed = false;
        for (std::size_t offset = 0; offset < 12 && !completed; ++offset) {
            FailingUpstream upstream;
            {
                GlobalContext global(&upstream);
                auto user = global.make_user_context();
                auto& input = user->make<Vector<String>>();
                for (auto character : {'x', 'y', 'z'}) {
                    input.emplace_back(65536, character);
                }
                auto reference = user->make_global<EntityDescriptor>("Customer");
                auto& descriptor = user->get(reference);
                descriptor.mask_fields({"previous"});
                descriptor.audit_mask_fields_declared = declared;
                auto issued = global.allocation_stats().issued_objects;
                upstream.fail_at = upstream.attempts + offset;
                bool failed = false;
                try {
                    descriptor.mask_fields(input);
                } catch (const std::bad_alloc&) {
                    failed = true;
                    ++failures;
                }
                upstream.fail_at.reset();
                if (failed) {
                    check(descriptor.audit_mask_fields.size() == 1 &&
                              descriptor.audit_mask_fields[0] == "previous" &&
                              descriptor.audit_mask_fields_declared == declared,
                          "allocation failure preserves the entire previous policy");
                    descriptor.mask_fields(input);
                } else {
                    completed = true;
                }
                check(descriptor.audit_mask_fields.size() == 3 &&
                          descriptor.audit_mask_fields[2].size() == 65536 &&
                          descriptor.audit_mask_fields[2].front() == 'z' &&
                          descriptor.audit_mask_fields_declared &&
                          global.allocation_stats().issued_objects == issued,
                      "retry publishes complete fields without another object identity");
                user.reset();
                auto next = global.make_user_context();
                auto& retained = next->get(next->rebind_global(reference));
                check(retained.audit_mask_fields[0].size() == 65536 &&
                          retained.audit_mask_fields[1].front() == 'y',
                      "failure recovery retains no source User content");
            }
            check(upstream.retained_bytes == 0 && upstream.retained_blocks == 0,
                  "all failed and successful blocks reclaimed");
        }
        check(failures >= 2 && completed,
              "fault injection crosses multiple field-copy allocations");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        timestamp_container_contracts();
        dynamic_mask_lifetimes();
        mask_allocation_failures();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
