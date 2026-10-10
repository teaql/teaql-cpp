#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_graph_decoder.hpp>

namespace teaql {
EntityRuntimeState UserContext::entity_runtime_state() const {
    check_public();
    if (!entity_runtime_state_) {
        entity_runtime_state_ =
            &const_cast<detail::ContextArena&>(memory_).make_payload<EntityRuntimeState>();
    }
    return EntityRuntimeState::fresh_with_shared_graph(memory_resource(), *entity_runtime_state_);
}

void UserContext::set_entity_graph_decoder_registry(
    const InMemoryEntityGraphDecoderRegistry& registry) {
    ensure_mutable();
    graph_decoders_ = &make<InMemoryEntityGraphDecoderRegistry>(registry);
}

std::optional<EntityBehaviorBorrow>
UserContext::entity_data_service_behavior(std::string_view entity) const {
    ensure_open();
    Arena::Quiescence active;
    if (!memory_.closing()) {
        active.attach(const_cast<detail::ContextArena&>(memory_), nullptr, [](void*) noexcept {
            std::terminate();
        });
    }
    if (entity_behaviors_borrow_) {
        (void)entity_behaviors_borrow_->get();
    }
    return entity_behaviors_ ? entity_behaviors_->behavior(memory_resource(), entity)
                             : std::nullopt;
}
}
