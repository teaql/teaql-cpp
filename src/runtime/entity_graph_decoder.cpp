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

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_compact(std::string_view entity,
                                                   CompactRow row,
                                                   const EntityRuntimeState& root,
                                                   EntityGraphBuilder& graph) const {
    auto found = compact_.find(entity);
    if (found == compact_.end())
        return missing(graph.memory_resource(), entity, "");
    validate_domain(root, graph);
    return found->second(std::move(row), root, graph);
}

EntityGraphDecodeResult
InMemoryEntityGraphDecoderRegistry::decode_compact_batch(std::string_view entity,
                                                         Vector<CompactRow> rows,
                                                         const EntityRuntimeState& root,
                                                         EntityGraphBuilder& graph) const {
    auto found = batch_.find(entity);
    if (found == batch_.end())
        return missing(graph.memory_resource(), entity, "batch ");
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
    auto found = list_.find(entity);
    if (found == list_.end())
        return missing(graph.memory_resource(), entity, "list ");
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
    if (found == option_.end())
        return missing(graph.memory_resource(), entity, "option ");
    validate_domain(root, graph);
    return found->second(std::move(rows), root, graph, owner, id, relation);
}
}
