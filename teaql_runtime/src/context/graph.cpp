#include <teaql/runtime/context.hpp>
#include <teaql/runtime/graph.hpp>

namespace teaql {
std::span<const GraphNode> UserContext::initial_graphs() const {
    ensure_open();
    return initial_graphs_ ? std::span<const GraphNode>(*initial_graphs_)
                           : std::span<const GraphNode>{};
}

std::span<const GraphNode> UserContext::root_graphs() const {
    ensure_open();
    return root_graphs_ ? std::span<const GraphNode>(*root_graphs_) : std::span<const GraphNode>{};
}

void UserContext::set_initial_graphs(std::span<const GraphNode> graphs) {
    ensure_mutable();
    initial_graphs_ = &make<Vector<GraphNode>>(graphs.begin(), graphs.end());
}

void UserContext::set_root_graphs(std::span<const GraphNode> graphs) {
    ensure_mutable();
    root_graphs_ = &make<Vector<GraphNode>>(graphs.begin(), graphs.end());
}
}
