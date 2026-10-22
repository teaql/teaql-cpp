#include "memory_data_service.hpp"
#include <teaql/runtime/registry_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace teaql;
using Service = original_memory::MemoryDataService;

template <class M>
concept ServiceDebugMetadata =
    requires(const Service& service, DebugWriter& writer, const M& metadata) {
        service.write_debug_to(writer, metadata);
    };
static_assert(ServiceDebugMetadata<InMemoryMetadataStore>);
static_assert(!ServiceDebugMetadata<MetadataStore>);

static void
emit(Memory r, std::string_view name, std::string_view debug, const String* display = nullptr) {
    auto out = detail::model_json(r);
    detail::model_put(out, "case", name);
    detail::model_put(out, "debug", debug);
    if (display) {
        detail::model_put(out, "display", *display);
    }
    std::cout << out.dump(r) << '\n';
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    if (argc == 2 && std::string_view(argv[1]) == "--binding-test") {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        auto& other = user_context->make<InMemoryMetadataStore>();
        auto& service = user_context->make<Service>(metadata);
        String result(r);
        DebugWriter writer(result, {});
        try {
            service.write_debug_to(writer, other);
        } catch (const std::invalid_argument&) {
            return result.empty() ? 0 : 2;
        }
        return 1;
    }
    for (unsigned kind = 0; kind < 3; ++kind) {
        for (bool pretty : {false, true}) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            using Error = original_memory::MemoryDataServiceError;
            Error error(std::allocator_arg,
                        Allocator(r),
                        static_cast<Error::Kind>(kind),
                        "quoted \"text\"\n");
            auto debug = debug_string(r, error, DebugOptions{.pretty = pretty});
            auto display = error.to_string(r);
            emit(r,
                 String("error_", r) + debug_string(r, kind) + "_" + (pretty ? "1" : "0"),
                 debug,
                 &display);
        }
    }
    for (unsigned mode = 0; mode < 3; ++mode) {
        for (bool pretty : {false, true}) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto& metadata = user_context->make<InMemoryMetadataStore>();
            metadata.register_entity(EntityDescriptor(std::allocator_arg, Allocator(r), "Item"));
            auto& service = user_context->make<Service>(metadata);
            if (mode != 0) {
                Vector<CompactRow> rows(r);
                Record fields(r);
                fields.emplace("id", Value(r, std::int64_t(1)));
                fields.emplace("version", Value(r, INT64_MAX));
                fields.emplace("name", Value(r, "line\nquoted\""));
                rows.emplace_back(std::move(fields));
                service.seed("Item", rows);
            }
            if (mode == 2) {
                UpdateCommand update(
                    std::allocator_arg, Allocator(r), "Item", Value(r, std::int64_t(1)));
                update.expected_version_value = INT64_MAX;
                bool panicked = false;
                try {
                    (void)service.update(update);
                } catch (const original_memory::MemoryArithmeticPanic&) {
                    panicked = true;
                }
                if (!panicked) {
                    throw std::runtime_error("expected arithmetic panic");
                }
            }
            String debug(r);
            DebugWriter writer(debug, DebugOptions{.pretty = pretty});
            service.write_debug_to(writer, metadata);
            emit(r,
                 String("service_", r) + debug_string(r, mode) + "_" + (pretty ? "1" : "0"),
                 debug);
        }
    }
}
