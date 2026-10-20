#include <teaql/data_service/model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

struct TraceView {
    std::span<const TraceNode> nodes;
};

static void write_debug(DebugWriter& writer, const TraceView& value) {
    writer.sequence(value.nodes, [](auto& out, const auto& node) {
        write_debug(out, node);
    });
}

static String trace_debug(Memory resource, std::span<const TraceNode> nodes) {
    return debug_string(resource, TraceView{nodes});
}

static TraceChain trace(Memory resource, const Json& input) {
    TraceChain frames(resource);
    for (const auto& node : std::get<Json::Array>(field(input, "trace").data)) {
        std::optional<std::uint64_t> id;
        if (!std::holds_alternative<std::monostate>(field(node, "id").data)) {
            id = number<std::uint64_t>(field(node, "id"));
        }
        frames.emplace_back(TraceKind(std::get<std::uint64_t>(field(node, "kind").data)),
                            text(field(node, "name")),
                            text(field(node, "comment")),
                            id);
    }
    return frames;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto resource = user_context.memory_resource();
    auto entity = text(field(input, "entity"));
    auto comment = text(field(input, "comment"));
    auto frames = trace(resource, input);
    Json::Object output(resource);
    if (text(field(input, "kind")) == "query") {
        SelectQuery query(std::allocator_arg, Allocator(resource), entity);
        query.comment("builder comment").purpose("builder purpose");
        query.trace_chain = std::move(frames);
        auto intent = QueryIntent::create(resource, comment, text(field(input, "purpose")));
        if (!intent) {
            throw std::runtime_error("The observation requires valid owned intent");
        }
        QueryRequest request(std::allocator_arg,
                             Allocator(resource),
                             query,
                             intent.value(),
                             std::get<bool>(field(input, "debug").data),
                             std::get<bool>(field(input, "metadata").data));
        output.emplace("stored_before", trace_debug(resource, request.trace_chain));
        output.emplace("execution", trace_debug(resource, request.execution_trace_chain()));
        request.query.entity = "ChangedBuilder";
        request.query.comment("changed builder comment").purpose("changed builder purpose");
        request.query.trace_chain.clear();
        output.emplace("stored_after", trace_debug(resource, request.trace_chain));
        output.emplace("after_builder_change",
                       trace_debug(resource, request.execution_trace_chain()));
        output.emplace("comment", Json(resource, request.comment()));
        output.emplace("purpose", Json(resource, request.purpose()));
        output.emplace("debug", Json(resource, request.capture_debug_query));
        output.emplace("metadata", Json(resource, request.capture_execution_metadata));
        return Json(resource, std::move(output));
    }
    auto id = ::value(resource, field(input, "id"));
    auto command = [&]() -> MutationCommand {
        auto kind = text(field(input, "kind"));
        if (kind == "insert") {
            InsertCommand value(std::allocator_arg, Allocator(resource), entity);
            if (std::get<bool>(field(input, "include_id").data)) {
                value.values.fields.emplace(String("id", resource), id);
            }
            value.trace_chain = std::move(frames);
            return MutationCommand(std::allocator_arg, Allocator(resource), std::move(value));
        }
        if (kind == "update") {
            UpdateCommand value(std::allocator_arg, Allocator(resource), entity, id);
            value.trace_chain = std::move(frames);
            return MutationCommand(std::allocator_arg, Allocator(resource), std::move(value));
        }
        if (kind == "delete") {
            DeleteCommand value(std::allocator_arg, Allocator(resource), entity, id);
            value.trace_chain = std::move(frames);
            return MutationCommand(std::allocator_arg, Allocator(resource), std::move(value));
        }
        if (kind == "recover") {
            RecoverCommand value(std::allocator_arg, Allocator(resource), entity, id, -3);
            value.trace_chain = std::move(frames);
            return MutationCommand(std::allocator_arg, Allocator(resource), std::move(value));
        }
        if (kind != "batch") {
            throw std::runtime_error("Unknown request observation");
        }
        InsertCommand insert(std::allocator_arg, Allocator(resource), "Child");
        insert.value("id", std::uint64_t(7));
        auto child = MutationRequest::create(resource, insert, "child owned reason");
        MutationRequest::Batch nested_children(resource);
        nested_children.emplace_back(child.value());
        auto nested =
            MutationRequest::create(resource, std::move(nested_children), "nested owned reason");
        MutationRequest::Batch children(resource);
        children.emplace_back(child.value());
        children.emplace_back(std::move(nested).value());
        return MutationCommand(std::allocator_arg, Allocator(resource), std::move(children));
    }();
    auto result = std::move(command).request(resource, comment);
    if (!result) {
        throw std::runtime_error("The observation requires a valid mutation reason");
    }
    auto& request = result.value();
    output.emplace("stored_before", trace_debug(resource, request.trace_chain()));
    output.emplace("execution", trace_debug(resource, request.execution_trace_chain()));
    output.emplace("stored_after", trace_debug(resource, request.trace_chain()));
    output.emplace("comment", Json(resource, request.comment()));
    Json::Array children(resource);
    if (auto* batch = std::get_if<MutationRequest::Batch>(&request.data)) {
        for (const auto& child : *batch) {
            children.emplace_back(trace_debug(resource, child.execution_trace_chain()));
        }
    }
    output.emplace("children", Json(resource, std::move(children)));
    return Json(resource, std::move(output));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid observation input");
        }
        std::cout << observe(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
}
