#include <teaql/runtime.hpp>

void check_graph_debug(teaql::Memory memory, const teaql::GraphNode& node) {
    (void)teaql::debug_string(memory, node);
}
