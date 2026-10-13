#pragma once
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>
#include <algorithm>

namespace teaql {
using EntityGraphDecodeResult = Result<std::monostate, EntityError>;

// Each mode retains a separate decoder map and source failure/installation order.
class InMemoryEntityGraphDecoderRegistry {
public:
    using allocator_type = Allocator;

    InMemoryEntityGraphDecoderRegistry(std::allocator_arg_t, Allocator a)
        : compact_(a.resource()), batch_(a.resource()), list_(a.resource()), option_(a.resource()),
          json_(a.resource()) {}

    InMemoryEntityGraphDecoderRegistry(std::allocator_arg_t,
                                       Allocator a,
                                       const InMemoryEntityGraphDecoderRegistry& other)
        : compact_(other.compact_, a.resource()), batch_(other.batch_, a.resource()),
          list_(other.list_, a.resource()), option_(other.option_, a.resource()),
          json_(other.json_, a.resource()) {}

    InMemoryEntityGraphDecoderRegistry(const InMemoryEntityGraphDecoderRegistry&) = delete;

    Memory memory_resource() const noexcept {
        return compact_.get_allocator().resource();
    }

    bool contains(std::string_view entity) const {
        return compact_.contains(entity);
    }

    struct JsonEdgeRequest {
        std::string_view owner;
        std::uint64_t id;
        std::string_view relation;
        bool many;
    };

    using JsonRows = Vector<std::pair<std::uint64_t, CompactRow>>;
    Result<bool, EntityError> json_capability(const EntityDescriptor&) const;
    EntityGraphDecodeResult decode_json_edge(std::string_view entity,
                                             JsonRows,
                                             const EntityRuntimeState&,
                                             EntityGraphBuilder&,
                                             JsonEdgeRequest) const;

    template <IdentifiableEntity T>
    void register_entity() {
        auto r = memory_resource();
        compact_.insert_or_assign(String(T::ENTITY_NAME, r), &compact<T>);
        batch_.insert_or_assign(String(T::ENTITY_NAME, r), &batch<T>);
        list_.insert_or_assign(String(T::ENTITY_NAME, r), &list<T>);
        option_.insert_or_assign(String(T::ENTITY_NAME, r), &option<T>);
        json_.insert_or_assign(String(T::ENTITY_NAME, r),
                               JsonReadDecoder{&json_capability_for<T>, &json_edge<T>});
    }

    EntityGraphDecodeResult decode_compact(std::string_view,
                                           CompactRow,
                                           const EntityRuntimeState&,
                                           EntityGraphBuilder&) const;
    EntityGraphDecodeResult decode_compact_batch(std::string_view,
                                                 Vector<CompactRow>,
                                                 const EntityRuntimeState&,
                                                 EntityGraphBuilder&) const;
    EntityGraphDecodeResult decode_compact_list(std::string_view,
                                                Vector<CompactRow>,
                                                const EntityRuntimeState&,
                                                EntityGraphBuilder&,
                                                std::string_view owner,
                                                std::uint64_t id,
                                                std::string_view relation) const;
    EntityGraphDecodeResult decode_compact_smart_list(std::string_view,
                                                      SmartList<CompactRow>,
                                                      const EntityRuntimeState&,
                                                      EntityGraphBuilder&,
                                                      std::string_view owner,
                                                      std::uint64_t id,
                                                      std::string_view relation) const;
    EntityGraphDecodeResult decode_compact_option(std::string_view,
                                                  Vector<CompactRow>,
                                                  const EntityRuntimeState&,
                                                  EntityGraphBuilder&,
                                                  std::string_view owner,
                                                  std::uint64_t id,
                                                  std::string_view relation) const;

private:
    using CompactDecoder = EntityGraphDecodeResult (*)(CompactRow,
                                                       const EntityRuntimeState&,
                                                       EntityGraphBuilder&);
    using BatchDecoder = EntityGraphDecodeResult (*)(Vector<CompactRow>,
                                                     const EntityRuntimeState&,
                                                     EntityGraphBuilder&);
    using RelationDecoder = EntityGraphDecodeResult (*)(Vector<CompactRow>,
                                                        const EntityRuntimeState&,
                                                        EntityGraphBuilder&,
                                                        std::string_view,
                                                        std::uint64_t,
                                                        std::string_view);
    using ListDecoder = EntityGraphDecodeResult (*)(SmartList<CompactRow>,
                                                    const EntityRuntimeState&,
                                                    EntityGraphBuilder&,
                                                    std::string_view,
                                                    std::uint64_t,
                                                    std::string_view);
    std::pmr::map<String, CompactDecoder, std::less<>> compact_;
    std::pmr::map<String, BatchDecoder, std::less<>> batch_;
    std::pmr::map<String, ListDecoder, std::less<>> list_;
    std::pmr::map<String, RelationDecoder, std::less<>> option_;

    using JsonCapability = Result<bool, EntityError> (*)(Memory, const EntityDescriptor&);
    using JsonDecoder = EntityGraphDecodeResult (*)(JsonRows,
                                                    const EntityRuntimeState&,
                                                    EntityGraphBuilder&,
                                                    JsonEdgeRequest);

    struct JsonReadDecoder {
        JsonCapability capability;
        JsonDecoder edge;
    };

    std::pmr::map<String, JsonReadDecoder, std::less<>> json_;

    template <IdentifiableEntity T>
    static Result<bool, EntityError> json_capability_for(Memory r,
                                                         const EntityDescriptor& installed) {
        using R = Result<bool, EntityError>;
        auto expected = T::entity_descriptor(r);
        auto mismatch = [&] {
            return R::err(
                r,
                EntityError(
                    std::allocator_arg,
                    Allocator(r),
                    T::ENTITY_NAME,
                    "JSON_ENTITY_INPUT: installed graph metadata does not match its typed decoder"));
        };
        if (installed.properties.size() != expected.properties.size() ||
            std::ranges::any_of(expected.properties,
                                [&](const auto& property) {
                                    return std::ranges::find(installed.properties, property) ==
                                           installed.properties.end();
                                }) ||
            installed.relations.size() != expected.relations.size() ||
            std::ranges::any_of(expected.relations, [&](const auto& relation) {
                return std::ranges::find(installed.relations, relation) ==
                       installed.relations.end();
            })) {
            return mismatch();
        }
        if constexpr (requires { T::field_layout(r); }) {
            auto&& layout = T::field_layout(r);
            if constexpr (requires {
                              layout.error();
                              layout.value();
                          }) {
                if (!layout) {
                    if constexpr (std::same_as<std::remove_cvref_t<decltype(layout.error())>,
                                               EntityError>) {
                        return R::err(r, detail::clone_in(r, layout.error()));
                    } else {
                        return R::err(
                            r,
                            EntityError(
                                std::allocator_arg, Allocator(r), T::ENTITY_NAME, layout.error()));
                    }
                }
                if constexpr (requires { layout.value().has_value(); }) {
                    if (!layout.value()) {
                        return mismatch();
                    }
                }
            } else if constexpr (requires { layout.has_value(); }) {
                if (!layout) {
                    return mismatch();
                }
            }
        } else {
            return mismatch();
        }
        return R::ok(r, T::supports_dynamic_property_load());
    }

    template <IdentifiableEntity T>
    static EntityGraphDecodeResult json_edge(JsonRows rows,
                                             const EntityRuntimeState& root,
                                             EntityGraphBuilder& graph,
                                             JsonEdgeRequest edge) {
        auto r = graph.memory_resource();
        detail::ReadHydrationShapes shapes(r);
        if (edge.many) {
            SmartList<T> list(std::allocator_arg, Allocator(r));
            list.data.reserve(rows.size());
            for (auto& [view, row] : rows) {
                auto scoped =
                    detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root).with_json_view(
                        view);
                auto entity = detail::decode_compact_row_with_read_metadata<T>(
                    r, std::move(row), scoped, shapes);
                if (!entity) {
                    return EntityGraphDecodeResult::err(r, std::move(entity).error());
                }
                list.data.emplace_back(std::move(entity).value());
            }
            list.loaded();
            graph.install_json_relation_list(
                root.json_view_id(), edge.owner, edge.id, edge.relation, std::move(list));
        } else {
            std::optional<T> value;
            if (!rows.empty()) {
                auto scoped =
                    detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root).with_json_view(
                        rows.front().first);
                auto entity = detail::decode_compact_row_with_read_metadata<T>(
                    r, std::move(rows.front().second), scoped, shapes);
                if (!entity) {
                    return EntityGraphDecodeResult::err(r, std::move(entity).error());
                }
                value.emplace(std::move(entity).value());
            }
            graph.install_typed_relation_option(
                root.json_view_id(), edge.owner, edge.id, edge.relation, std::move(value));
        }
        return EntityGraphDecodeResult::ok(r, {});
    }

    static void validate_domain(const EntityRuntimeState& root, const EntityGraphBuilder& graph) {
        if (root.memory_resource() != graph.memory_resource()) {
            throw std::invalid_argument(
                "Identity graph decoder root and builder must share their owning Context");
        }
    }

    template <IdentifiableEntity T>
    static EntityGraphDecodeResult
    compact(CompactRow row, const EntityRuntimeState& root, EntityGraphBuilder& graph) {
        auto r = graph.memory_resource();
        auto graph_root = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root);
        detail::ReadHydrationShapes shapes(r);
        auto entity =
            detail::decode_compact_row_with_read_metadata<T>(r, std::move(row), graph_root, shapes);
        if (!entity) {
            return EntityGraphDecodeResult::err(r, std::move(entity).error());
        }
        auto id = entity.value().id_value().try_u64();
        if (!id) {
            return EntityGraphDecodeResult::err(
                r,
                EntityError(std::allocator_arg,
                            Allocator(r),
                            T::ENTITY_NAME,
                            "identity graph requires a u64 entity id"));
        }
        graph.install(*id, std::move(entity).value());
        return EntityGraphDecodeResult::ok(r, {});
    }

    template <IdentifiableEntity T>
    static EntityGraphDecodeResult
    batch(Vector<CompactRow> rows, const EntityRuntimeState& root, EntityGraphBuilder& graph) {
        auto r = graph.memory_resource();
        auto graph_root = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root);
        detail::ReadHydrationShapes shapes(r);
        CompactRow::share_layouts(rows);
        for (auto& row : rows) {
            auto entity = detail::decode_compact_row_with_read_metadata<T>(
                r, std::move(row), graph_root, shapes);
            if (!entity) {
                return EntityGraphDecodeResult::err(r, std::move(entity).error());
            }
            auto id = entity.value().id_value().try_u64();
            if (!id) {
                return EntityGraphDecodeResult::err(
                    r,
                    EntityError(std::allocator_arg,
                                Allocator(r),
                                T::ENTITY_NAME,
                                "identity graph requires a u64 entity id"));
            }
            graph.install(*id, std::move(entity).value());
        }
        return EntityGraphDecodeResult::ok(r, {});
    }

    template <IdentifiableEntity T>
    static EntityGraphDecodeResult list(SmartList<CompactRow> rows,
                                        const EntityRuntimeState& root,
                                        EntityGraphBuilder& graph,
                                        std::string_view owner,
                                        std::uint64_t id,
                                        std::string_view relation) {
        auto r = graph.memory_resource();
        auto graph_root = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root);
        detail::ReadHydrationShapes shapes(r);
        SmartList<T> entities(std::allocator_arg, Allocator(r));
        entities.data.reserve(rows.data.size());
        CompactRow::share_layouts(rows.data);
        for (auto& row : rows.data) {
            auto entity = detail::decode_compact_row_with_read_metadata<T>(
                r, std::move(row), graph_root, shapes);
            if (!entity) {
                return EntityGraphDecodeResult::err(r, std::move(entity).error());
            }
            entities.data.emplace_back(std::move(entity).value());
        }
        entities.is_loaded = rows.is_loaded;
        entities.total_count = rows.total_count;
        entities.aggregations = std::move(rows.aggregations);
        entities.summary = std::move(rows.summary);
        entities.facets = std::move(rows.facets);
        graph.install_relation_list(owner, id, relation, std::move(entities));
        return EntityGraphDecodeResult::ok(r, {});
    }

    template <IdentifiableEntity T>
    static EntityGraphDecodeResult option(Vector<CompactRow> rows,
                                          const EntityRuntimeState& root,
                                          EntityGraphBuilder& graph,
                                          std::string_view owner,
                                          std::uint64_t id,
                                          std::string_view relation) {
        auto r = graph.memory_resource();
        auto graph_root = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root);
        detail::ReadHydrationShapes shapes(r);
        std::optional<T> value;
        if (!rows.empty()) {
            auto entity = detail::decode_compact_row_with_read_metadata<T>(
                r, std::move(rows.front()), graph_root, shapes);
            if (!entity) {
                return EntityGraphDecodeResult::err(r, std::move(entity).error());
            }
            value.emplace(std::move(entity).value());
        }
        graph.install_relation_option(owner, id, relation, std::move(value));
        return EntityGraphDecodeResult::ok(r, {});
    }
};
}
