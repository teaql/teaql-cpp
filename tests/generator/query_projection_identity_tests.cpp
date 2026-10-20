#include "../../examples/generated/include/cpp_examples.hpp"
#include <array>
#include <iostream>
using namespace teaql;

// Only query construction is exercised. No fixture decoder replaces the runtime.
struct ProjectionIdentityFixture : cpp_examples::Product {
    static constexpr std::array<std::string_view, 3> QUERY_PROJECTION_FIELDS{
        "id", "name", "version"};

    static EntityDescriptor entity_descriptor(Memory r) {
        auto descriptor = cpp_examples::Product::entity_descriptor(r);
        for (auto& field : descriptor.properties) {
            if (field.name == "name") {
                field.name = "name_field";
            }
        }
        return descriptor;
    }

    static Result<ProjectionIdentityFixture, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Projection fixture does not decode rows");
    }

    static Result<ProjectionIdentityFixture, EntityError>
    from_compact_row_with_context(Memory, CompactRow, AnyRef) {
        throw std::logic_error("Projection fixture does not decode rows");
    }
};

static_assert(Entity<ProjectionIdentityFixture>);

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::logic_error(message);
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        using Query = EntityQuery<ProjectionIdentityFixture, cpp_examples::DataServiceExecutor>;
        Query ordinary(*user_context), minimal(*user_context, true);
        check(ordinary.query().projection.size() == 5 && ordinary.query().projection[3] == "name",
              "Normal request projects source query names independently of metadata names");
        check(minimal.query().projection.size() == 2,
              "Minimal requests keep their independent identity-only projection");
        check(ordinary.query().predicate && minimal.query().predicate,
              "Separate projection names preserve the existing live-row predicate");
        auto retained = ordinary.copyToGlobal(*user_context);
        user_context.reset();
        check(global.live_user_bytes() == 0, "Source query storage must be released");
        bool expired = false;
        try {
            (void)ordinary.query();
        } catch (const std::exception&) {
            expired = true;
        }
        check(expired, "Ordinary request borrows expire after UserContext deletion");
        auto next_user_context = global.make_user_context();
        auto rebound = retained.rebindGlobal(*next_user_context);
        check(rebound.query().projection[3] == "name" &&
                  rebound.query().projection[3].get_allocator().resource() ==
                      global.memory_resource(),
              "Explicit retention preserves the projection and destination allocator");
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
