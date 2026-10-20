#pragma once
#include <teaql/runtime/entity_runtime.hpp>

namespace teaql {
class UserContext;

namespace detail {
EntityError json_input_error(Memory, std::string_view entity, std::string_view message);

// Operation-local geometry cache. It owns no data outside the caller's Context.
class JsonReadSession {
public:
    explicit JsonReadSession(const UserContext&);
    Result<const EntityDescriptor*, EntityError> descriptor(const EntityDescriptor&) const;
    Result<CompactRow, EntityError>
    decode(const EntityDescriptor&, const Json&, bool, std::optional<EntityRuntimeState>&);

private:
    const UserContext& context_;
    Memory resource_;
    std::pmr::map<Vector<String>, const ColumnLayout*> shapes_;
    CompactRow scalar_row(const EntityDescriptor&, const Json&, bool supports_dynamic, bool graph);
    CompactRow inferred_row(const EntityDescriptor&, const Json&, bool supports_dynamic);
    struct Plan;
    struct Parsed;
    Parsed parse_graph(const EntityDescriptor&, const Json&, Plan&, std::size_t depth);
    std::pair<std::uint64_t, CompactRow>
    install_edges(Parsed&&, const EntityRuntimeState&, EntityGraphBuilder&);
};

template <Entity T>
Result<std::monostate, EntityError> require_json_layout(Memory r) {
    using Output = Result<std::monostate, EntityError>;
    auto missing = [&] {
        return Output::err(
            r, json_input_error(r, T::ENTITY_NAME, "typed JSON requires generated field indexes"));
    };
    if constexpr (requires { T::field_layout(r); }) {
        auto&& layout = T::field_layout(r);
        if constexpr (requires {
                          layout.error();
                          layout.value();
                      }) {
            if (!layout) {
                if constexpr (std::same_as<std::remove_cvref_t<decltype(layout.error())>,
                                           EntityError>) {
                    return Output::err(r, clone_in(r, layout.error()));
                } else {
                    return Output::err(
                        r,
                        EntityError(
                            std::allocator_arg, Allocator(r), T::ENTITY_NAME, layout.error()));
                }
            }
            if constexpr (requires { layout.value().has_value(); }) {
                if (!layout.value()) {
                    return missing();
                }
            }
        } else if constexpr (requires { layout.has_value(); }) {
            if (!layout) {
                return missing();
            }
        }
    } else {
        return missing();
    }
    return Output::ok(r, std::monostate{});
}
}
}
