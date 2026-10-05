#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_graph_decoder.hpp>

namespace teaql::detail {
// Crate-private runtime boundary in Rust. The registry itself is public; these
// Context conveniences are reserved for the entity runtime.
struct ContextEntityAccess {
private:
    template <class F>
    static EntityGraphDecodeResult with_decoders(const UserContext& context, F&& call) {
        context.ensure_open();
        if (context.graph_decoders_) {
            return call(*context.graph_decoders_);
        }
        InMemoryEntityGraphDecoderRegistry empty(std::allocator_arg,
                                                 Allocator(context.memory_resource()));
        return call(empty);
    }

public:
    static void ensure_open(const UserContext& context) {
        context.ensure_open();
    }

    static bool has_metadata(const UserContext& context) {
        context.ensure_open();
        return context.metadata_ != nullptr;
    }

    template <class E>
    static Result<ResourceBorrow<E>, ContextError> require_executor(const UserContext& context) {
        context.ensure_open();
        return context.required<E>(context.resources_ ? context.resources_->find<E>() : nullptr,
                                   ContextError::Kind::MissingTypedResource,
                                   resource_type_name<E>());
    }

    static bool has_entity_graph_decoder(const UserContext& context, std::string_view entity) {
        context.ensure_open();
        return context.graph_decoders_ && context.graph_decoders_->contains(entity);
    }

    static const RequestPolicy* request_policy(const UserContext& context) {
        context.ensure_open();
        return context.request_policy_;
    }

    static EntityGraphDecodeResult decode_entity_graph_compact(const UserContext& context,
                                                               std::string_view entity,
                                                               CompactRow row,
                                                               const EntityRuntimeState& root,
                                                               EntityGraphBuilder& graph) {
        return with_decoders(context, [&](const auto& decoders) {
            return decoders.decode_compact(entity, std::move(row), root, graph);
        });
    }

    static EntityGraphDecodeResult decode_entity_graph_compact_batch(const UserContext& context,
                                                                     std::string_view entity,
                                                                     Vector<CompactRow> rows,
                                                                     const EntityRuntimeState& root,
                                                                     EntityGraphBuilder& graph) {
        return with_decoders(context, [&](const auto& decoders) {
            return decoders.decode_compact_batch(entity, std::move(rows), root, graph);
        });
    }

    static EntityGraphDecodeResult decode_entity_graph_compact_list(const UserContext& context,
                                                                    std::string_view entity,
                                                                    Vector<CompactRow> rows,
                                                                    const EntityRuntimeState& root,
                                                                    EntityGraphBuilder& graph,
                                                                    std::string_view owner,
                                                                    std::uint64_t id,
                                                                    std::string_view relation) {
        return with_decoders(context, [&](const auto& decoders) {
            return decoders.decode_compact_list(
                entity, std::move(rows), root, graph, owner, id, relation);
        });
    }

    static EntityGraphDecodeResult
    decode_entity_graph_compact_option(const UserContext& context,
                                       std::string_view entity,
                                       Vector<CompactRow> rows,
                                       const EntityRuntimeState& root,
                                       EntityGraphBuilder& graph,
                                       std::string_view owner,
                                       std::uint64_t id,
                                       std::string_view relation) {
        return with_decoders(context, [&](const auto& decoders) {
            return decoders.decode_compact_option(
                entity, std::move(rows), root, graph, owner, id, relation);
        });
    }
};
}
