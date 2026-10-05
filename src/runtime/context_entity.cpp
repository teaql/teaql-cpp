#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_graph_decoder.hpp>

namespace teaql {
void UserContext::set_entity_graph_decoder_registry(
    const InMemoryEntityGraphDecoderRegistry& registry) {
    ensure_mutable();
    graph_decoders_ = &make<InMemoryEntityGraphDecoderRegistry>(registry);
}

std::optional<EntityBehaviorBorrow>
UserContext::entity_data_service_behavior(std::string_view entity) const {
    ensure_open();
    return entity_behaviors_ ? entity_behaviors_->behavior(memory_resource(), entity)
                             : std::nullopt;
}
}
