#include "teaql_runtime/entity_runtime_contracts.hpp"
#include <iostream>

using namespace teaql;
using teaql::test::source_entity_runtime::TestEntity;

struct NotEntity : LedgerEntityDefaults<NotEntity> {
    Memory owner;

    Memory memory_resource() const {
        return owner;
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        return {};
    }
};

static_assert(!Entity<NotEntity> && !LedgerEntity<NotEntity>);

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    TestEntity model(std::allocator_arg, Allocator(r));
    NotEntity fake{{}, r};
#if defined(TEAQL_REJECT_TARGET)
    fake.include_pending_mutations_from(model);
#elif defined(TEAQL_REJECT_STATIC_TARGET)
    LedgerEntityDefaults<NotEntity>::compose(fake, model);
#elif defined(TEAQL_REJECT_SOURCE)
    model.include_pending_mutations_from(fake);
#else
    auto instance = model.include_pending_mutations_from(model);
    auto delegated = LedgerEntityDefaults<TestEntity>::compose(model, model);
    if (instance || delegated ||
        instance.error().kind != LedgerCompositionError::Kind::MissingTargetState ||
        delegated.error().kind != LedgerCompositionError::Kind::MissingTargetState) {
        throw std::runtime_error("Public LedgerEntity default lost target-state precedence");
    }
    std::cout << "ledger-entity: valid target and source; checked missing state\n";
#endif
}
