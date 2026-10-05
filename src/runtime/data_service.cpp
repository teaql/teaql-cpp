#include <teaql/runtime/detail/data_service.hpp>
#include <charconv>

namespace teaql::detail {
namespace {
struct ContextObserver {
    using allocator_type = Allocator;
    ResourceBorrow<const UserContext> context;

    ContextObserver(std::allocator_arg_t, Allocator a, const UserContext& context)
        : context(std::allocator_arg, a, context, resource_arena(context.memory_resource())) {}

    ContextObserver(std::allocator_arg_t, Allocator a, const ContextObserver& other)
        : context(std::allocator_arg, a, other.context) {}

    void operator()(ExecutionMetadata value) const {
        context.get().record_metadata_log(value);
    }
};
}

Vector<const EntityDescriptor*> UserContextMetadata::all_entities(Memory r) const {
    if (!resource_arena(context().memory_resource()).can_lend_to(resource_arena(r)))
        throw std::invalid_argument("Context metadata view cannot escape its owning Context");
    auto entities = context().all_entities();
    return Vector<const EntityDescriptor*>(entities.begin(), entities.end(), r);
}

std::optional<ExecutionObserver> UserContextMetadata::query_diagnostic_observer(Memory r) const {
    if (!capture_execution_metadata())
        return {};
    return ExecutionObserver(std::allocator_arg,
                             Allocator(r),
                             ContextObserver(std::allocator_arg, Allocator(r), context()));
}

std::optional<ExecutionObserver> UserContextMetadata::mutation_diagnostic_observer(Memory r) const {
    if (!context().sql_log_options().mutation)
        return {};
    return ExecutionObserver(std::allocator_arg,
                             Allocator(r),
                             ContextObserver(std::allocator_arg, Allocator(r), context()));
}

std::optional<String> resolve_final_comment(const UserContext& context,
                                            std::span<const TraceNode> trace,
                                            std::optional<std::string_view> comment) {
    auto r = context.memory_resource();
    String business(r);
    if (!trace.empty())
        for (const auto& node : trace) {
            if (!business.empty())
                business += " -> ";
            business += node.entity_type;
            business += '(';
            if (node.entity_id) {
                char buffer[32];
                auto result = std::to_chars(buffer, buffer + sizeof(buffer), *node.entity_id);
                business.append(buffer, result.ptr);
            } else
                business += "pending";
            business += "): ";
            business += node.comment;
        }
    else if (comment)
        business = *comment;
    auto user = context.user_identifier();
    String out(r);
    if (user && !user->empty()) {
        out += '[';
        out += *user;
        out += ']';
        if (!business.empty()) {
            out += ' ';
            out += business;
        }
    } else
        out = std::move(business);
    if (out.empty())
        return {};
    return out;
}
}

namespace teaql::detail {
void record_query_metadata(const UserContext& context,
                           SqlIntentRedactions* intent,
                           const ExecutionMetadata& value) {
    if (!intent) {
        context.record_metadata_log(value);
        return;
    }
    auto r = context.memory_resource();
    auto metadata = clone_in(r, value);
    auto inherit = [&](auto&& self, ExecutionMetadata& item) -> void {
        if (!item.statements.empty()) {
            for (auto& child : item.statements)
                self(self, child);
            return;
        }
        item.sql_log.intent_redactions.extend(*intent);
        *intent = SqlIntentRedactions::from_bindings(
            r,
            item.sql_log,
            item.params,
            item.parameterized_query ? std::string_view(*item.parameterized_query)
                                     : std::string_view{});
    };
    inherit(inherit, metadata);
    context.record_metadata_log(metadata);
}
}
