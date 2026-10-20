//! Import complete original entity/Checker files unchanged; full runtime assembly has a frozen error.
#[path = "../../generator/parity/checker_clock_composition_rust/lib/src/tenant/entity.rs"]
mod tenant_entity;
#[path = "../../generator/parity/checker_clock_composition_rust/lib/src/tenant/checker.rs"]
mod tenant_checker;
pub use tenant_entity::*;
pub use tenant_checker::*;
#[path = "../../generator/parity/checker_clock_composition_rust/lib/src/record/entity.rs"]
mod record_entity;
#[path = "../../generator/parity/checker_clock_composition_rust/lib/src/record/checker.rs"]
mod record_checker;
pub use record_entity::*;
pub use record_checker::*;
#[path = "../../generator/parity/checker_clock_composition_rust/lib/src/note/entity.rs"]
mod note_entity;
#[path = "../../generator/parity/checker_clock_composition_rust/lib/src/note/checker.rs"]
mod note_checker;
pub use note_entity::*;
pub use note_checker::*;
