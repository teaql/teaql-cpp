use teaql_core::{CompactRow, Entity, EntityDescriptor, EntityError, MutationValues, TeaqlEntity};
use teaql_runtime::{EntityRuntimeState, LedgerEntity};

struct Model;
impl TeaqlEntity for Model {
    const ENTITY_NAME: &'static str = "Model";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
}
impl Entity for Model {
    fn from_compact_row(_: CompactRow) -> Result<Self, EntityError> {
        Err(EntityError::new(Self::ENTITY_NAME, "not used by this compiler control"))
    }
    fn into_values(self) -> MutationValues { MutationValues::new() }
}
impl LedgerEntity for Model {
    fn entity_runtime_state(&self) -> Option<EntityRuntimeState> { None }
}
struct NotEntity;
#[cfg(feature = "reject_target")]
impl LedgerEntity for NotEntity {
    fn entity_runtime_state(&self) -> Option<EntityRuntimeState> { None }
}

fn main() {
    let model = Model;
    let fake = NotEntity;
    #[cfg(feature = "reject_target")]
    let _ = fake.include_pending_mutations_from(&model);
    #[cfg(feature = "reject_static_target")]
    let _ = LedgerEntity::include_pending_mutations_from(&fake, &model);
    #[cfg(feature = "reject_source")]
    let _ = model.include_pending_mutations_from(&fake);
    #[cfg(not(any(feature = "reject_target", feature = "reject_static_target", feature = "reject_source")))]
    {
        let _ = fake;
        assert_eq!(model.include_pending_mutations_from(&model),
            Err(teaql_runtime::LedgerCompositionError::MissingTargetState));
        assert_eq!(LedgerEntity::include_pending_mutations_from(&model, &model),
            Err(teaql_runtime::LedgerCompositionError::MissingTargetState));
        println!("ledger-entity: valid target and source; checked missing state");
    }
}
