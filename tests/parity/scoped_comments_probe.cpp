#include <teaql/runtime/context.hpp>
#include <teaql/runtime/graph.hpp>
#include <teaql/core/debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace teaql;

static Json record(Memory r, const TraceChain& nodes) {
    Json::Array result(r);
    for (const auto& node : nodes) {
        Json::Object fields(r);
        fields.emplace(String("kind", r), Json(r, debug_string(r, node.kind)));
        fields.emplace(String("entity", r), Json(r, node.entity_type));
        fields.emplace(String("id", r), node.entity_id ? Json(r, *node.entity_id) : Json(r));
        fields.emplace(String("comment", r), Json(r, node.comment));
        result.emplace_back(fields);
    }
    return Json(r, result);
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid scope input");
        }
        const ScopedCommentNode* stack = nullptr;
        const TraceScopeToken* persistent = nullptr;
        std::size_t index = 0;
        for (const auto& entry : std::get<Json::Array>(input->data)) {
            String entity("Entity", r);
            entity += std::to_string(index);
            TraceNode node(std::allocator_arg,
                           Allocator(r),
                           static_cast<TraceKind>(index % 9),
                           entity,
                           std::get<String>(entry.data),
                           index % 2 == 0 ? std::optional<std::uint64_t>(index) : std::nullopt);
            stack = &user_context->make<ScopedCommentNode>(stack, node);
            persistent = &user_context->make<TraceScopeToken>(persistent, node, index++);
        }
        Json::Object out(r);
        out.emplace(String("stack", r),
                    record(r, stack ? stack->to_trace_chain(r) : TraceChain(r)));
        out.emplace(String("persistent", r),
                    record(r, persistent ? persistent->recover_trace_chain(r) : TraceChain(r)));
        std::cout << Json(r, out).dump(r) << '\n';
    }
}
