#include <teaql/runtime/entity_graph_decoder.hpp>

namespace teaql {
namespace {
EntityGraphDecodeResult missing(Memory r, std::string_view entity, std::string_view mode) {
    String message("entity has no compact identity graph ", r);
    message += mode;
    message += "decoder in RuntimeModule";
    return EntityGraphDecodeResult::err(
        r, EntityError(std::allocator_arg, Allocator(r), entity, message));
}
}

Result<bool, EntityError>
InMemoryEntityGraphDecoderRegistry::json_capability(const EntityDescriptor& descriptor) const {
    auto r = memory_resource();
    auto found = json_.find(descriptor.name);
    if (found == json_.end()) {
        return Result<bool, EntityError>::err(
            r,
            EntityError(std::allocator_arg,
                        Allocator(r),
                        descriptor.name,
                        "JSON_ENTITY_INPUT: graph type has no installed typed decoder"));
    }
    return found->second.capability(r, descriptor);
}

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_json_edge(std::string_view entity,
                                                     JsonRows rows,
                                                     const EntityRuntimeState& root,
                                                     EntityGraphBuilder& graph,
                                                     JsonEdgeRequest edge) const {
    auto found = json_.find(entity);
    if (found == json_.end()) {
        auto r = graph.memory_resource();
        return EntityGraphDecodeResult::err(
            r,
            EntityError(std::allocator_arg,
                        Allocator(r),
                        entity,
                        "JSON_ENTITY_INPUT: graph type has no installed typed decoder"));
    }
    validate_domain(root, graph);
    return found->second.edge(std::move(rows), root, graph, edge);
}

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_compact(std::string_view entity,
                                                   CompactRow row,
                                                   const EntityRuntimeState& root,
                                                   EntityGraphBuilder& graph) const {
    auto found = compact_.find(entity);
    if (found == compact_.end()) {
        return missing(graph.memory_resource(), entity, "");
    }
    validate_domain(root, graph);
    return found->second(std::move(row), root, graph);
}

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_compact_batch(std::string_view entity,
                                                         Vector<CompactRow> rows,
                                                         const EntityRuntimeState& root,
                                                         EntityGraphBuilder& graph) const {
    auto found = batch_.find(entity);
    if (found == batch_.end()) {
        return missing(graph.memory_resource(), entity, "batch ");
    }
    validate_domain(root, graph);
    return found->second(std::move(rows), root, graph);
}

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_compact_list(std::string_view entity,
                                                        Vector<CompactRow> rows,
                                                        const EntityRuntimeState& root,
                                                        EntityGraphBuilder& graph,
                                                        std::string_view owner,
                                                        std::uint64_t id,
                                                        std::string_view relation) const {
    SmartList<CompactRow> list(
        std::allocator_arg, Allocator(graph.memory_resource()), std::move(rows));
    return decode_compact_smart_list(entity, std::move(list), root, graph, owner, id, relation);
}

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_compact_smart_list(std::string_view entity,
                                                              SmartList<CompactRow> rows,
                                                              const EntityRuntimeState& root,
                                                              EntityGraphBuilder& graph,
                                                              std::string_view owner,
                                                              std::uint64_t id,
                                                              std::string_view relation) const {
    auto found = list_.find(entity);
    if (found == list_.end()) {
        return missing(graph.memory_resource(), entity, "list ");
    }
    validate_domain(root, graph);
    return found->second(std::move(rows), root, graph, owner, id, relation);
}

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_compact_option(std::string_view entity,
                                                          Vector<CompactRow> rows,
                                                          const EntityRuntimeState& root,
                                                          EntityGraphBuilder& graph,
                                                          std::string_view owner,
                                                          std::uint64_t id,
                                                          std::string_view relation) const {
    auto found = option_.find(entity);
    if (found == option_.end()) {
        return missing(graph.memory_resource(), entity, "option ");
    }
    validate_domain(root, graph);
    return found->second(std::move(rows), root, graph, owner, id, relation);
}
}
