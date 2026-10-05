#pragma once
#include <teaql/runtime/entity_runtime.hpp>

namespace teaql {
using EntityGraphDecodeResult = Result<std::monostate, EntityError>;

// Each mode retains a separate decoder map and source failure/installation order.
class InMemoryEntityGraphDecoderRegistry {
public:
    using allocator_type = Allocator;

    InMemoryEntityGraphDecoderRegistry(std::allocator_arg_t, Allocator a)
        : compact_(a.resource()), batch_(a.resource()), list_(a.resource()), option_(a.resource()) {
    }

    InMemoryEntityGraphDecoderRegistry(std::allocator_arg_t,
                                       Allocator a,
                                       const InMemoryEntityGraphDecoderRegistry& other)
        : compact_(other.compact_, a.resource()), batch_(other.batch_, a.resource()),
          list_(other.list_, a.resource()), option_(other.option_, a.resource()) {}

    InMemoryEntityGraphDecoderRegistry(const InMemoryEntityGraphDecoderRegistry&) = delete;

    Memory memory_resource() const noexcept {
        return compact_.get_allocator().resource();
    }

    bool contains(std::string_view entity) const {
        return compact_.contains(entity);
    }

    template <IdentifiableEntity T>
    void register_entity() {
        auto r = memory_resource();
        compact_.insert_or_assign(String(T::ENTITY_NAME, r), &compact<T>);
        batch_.insert_or_assign(String(T::ENTITY_NAME, r), &batch<T>);
        list_.insert_or_assign(String(T::ENTITY_NAME, r), &list<T>);
        option_.insert_or_assign(String(T::ENTITY_NAME, r), &option<T>);
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
    std::pmr::map<String, CompactDecoder, std::less<>> compact_;
    std::pmr::map<String, BatchDecoder, std::less<>> batch_;
    std::pmr::map<String, RelationDecoder, std::less<>> list_, option_;

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
        auto entity = T::from_compact_row_with_context(r, std::move(row), AnyRef(graph_root));
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
        for (auto& row : rows) {
            auto entity = T::from_compact_row_with_context(r, std::move(row), AnyRef(graph_root));
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
    static EntityGraphDecodeResult list(Vector<CompactRow> rows,
                                        const EntityRuntimeState& root,
                                        EntityGraphBuilder& graph,
                                        std::string_view owner,
                                        std::uint64_t id,
                                        std::string_view relation) {
        auto r = graph.memory_resource();
        auto graph_root = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root);
        SmartList<T> entities(std::allocator_arg, Allocator(r));
        entities.data.reserve(rows.size());
        for (auto& row : rows) {
            auto entity = T::from_compact_row_with_context(r, std::move(row), AnyRef(graph_root));
            if (!entity) {
                return EntityGraphDecodeResult::err(r, std::move(entity).error());
            }
            entities.data.emplace_back(std::move(entity).value());
        }
        entities.loaded();
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
        std::optional<T> value;
        if (!rows.empty()) {
            auto entity =
                T::from_compact_row_with_context(r, std::move(rows.front()), AnyRef(graph_root));
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
