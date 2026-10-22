#include "memory_data_service.hpp"
#include <teaql/runtime/registry_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <latch>
#include <thread>
#include <iostream>
using namespace teaql;
using Service = original_memory::MemoryDataService;

namespace original_memory {
struct MemoryDebugAccess {
    static auto lock(const MemoryDataService& service) {
        return std::unique_lock(service.mutex_);
    }
};
}

struct InjectedFailure {};

// Test-only fault injector: every successful allocation still belongs to the
// underlying Context. This wrapper owns no blocks and outlives its output string.
class FailingResource final : public std::pmr::memory_resource {
public:
    Memory owner;
    const String* output = nullptr;
    std::string_view marker;

    FailingResource(Memory r, std::string_view marker) : owner(r), marker(marker) {}

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (output && output->find(marker) != String::npos) {
            throw InjectedFailure{};
        }
        return owner->allocate(bytes, alignment);
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        owner->deallocate(pointer, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static Service& setup(UserContext& user_context, InMemoryMetadataStore& metadata) {
    auto r = user_context.memory_resource();
    metadata.register_entity(EntityDescriptor(std::allocator_arg, Allocator(r), "Item"));
    auto& service = user_context.make<Service>(metadata);
    Record fields(r);
    fields.emplace("name", Value(r, String(4096, 'x', r)));
    Vector<CompactRow> rows(r);
    rows.emplace_back(std::move(fields));
    service.seed("Item", rows);
    return service;
}

static Json after(Memory r, Service& service) {
    SelectQuery query(std::allocator_arg, Allocator(r), "Item");
    auto result = service.fetch_all(query);
    auto out = detail::model_json(r);
    if (result) {
        detail::model_put(out, "rows", std::uint64_t(result.value().size()));
    } else {
        detail::model_put(out, "error", result.error().to_string(r));
    }
    return out;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (bool pretty : {false, true}) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        auto& service = setup(*user_context, metadata);
        String debug(r);
        {
            std::latch acquired(1), release(1);
            std::jthread worker([&] {
                auto lock = original_memory::MemoryDebugAccess::lock(service);
                acquired.count_down();
                release.wait();
            });

            struct Release {
                std::latch& latch;

                ~Release() {
                    latch.count_down();
                }
            } finish{release};

            acquired.wait();
            DebugWriter writer(debug, DebugOptions{.pretty = pretty});
            service.write_debug_to(writer, metadata);
        }
        auto out = after(r, service);
        detail::model_put(out, "case", pretty ? "locked_1" : "locked_0");
        detail::model_put(out, "debug", debug);
        std::cout << out.dump(r) << '\n';
    }
    for (unsigned mode = 0; mode < 3; ++mode) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        auto& service = setup(*user_context, metadata);
        String baseline(r);
        DebugWriter baseline_writer(baseline, {});
        service.write_debug_to(baseline_writer, metadata);
        FailingResource failures(r,
                                 mode == 0   ? "MemoryData"
                                 : mode == 1 ? "Mutex { data:"
                                             : "loaded_relations: None }]}, poisoned:");
        String rendered(&failures);
        if (mode == 2) {
            rendered.reserve(baseline.size() - 1);
            if (rendered.capacity() >= baseline.size()) {
                throw std::runtime_error("fault probe requires growth at final diagnostic suffix");
            }
        }
        failures.output = &rendered;
        bool injected = false;
        try {
            DebugWriter writer(rendered, {});
            service.write_debug_to(writer, metadata);
        } catch (const InjectedFailure&) {
            injected = true;
        }
        auto out = detail::model_json(r);
        detail::model_put(out, "case", String("failure_", r) + debug_string(r, mode));
        detail::model_put(out, "injected", injected);
        detail::model_put(out, "after", after(r, service));
        std::cout << out.dump(r) << '\n';
    }
}
