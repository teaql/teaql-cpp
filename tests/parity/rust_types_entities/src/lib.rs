//! Controlled entity/Checker oracle: compile unmodified original generated files.
//! Complete types/optional-types crates fail in request/runtime scaffolds.
//! This harness does not qualify those scaffolds or repair the frozen source.
#[path = "../../../generator/parity/types_rust/lib/src/person/entity.rs"]
mod person;
#[path = "../../../generator/parity/types_rust/lib/src/device/entity.rs"]
mod device;
#[path = "../../../generator/parity/types_rust/lib/src/order_status/entity.rs"]
mod order_status;
#[path = "../../../generator/parity/types_rust/lib/src/category/entity.rs"]
mod category;
#[path = "../../../generator/parity/types_rust/lib/src/child/entity.rs"]
mod child;
#[path = "../../../generator/parity/types_rust/lib/src/inventory/entity.rs"]
mod inventory;
pub use person::Person;
pub use device::Device;
pub use order_status::OrderStatus;
pub use category::Category;
pub use child::Child;
pub use inventory::Inventory;
#[path = "../../../generator/parity/optional_types_rust/lib/src/reading/entity.rs"]
mod reading;
#[path = "../../../generator/parity/optional_types_rust/lib/src/reading/checker.rs"]
mod reading_checker;
pub use reading::Reading;
pub use reading_checker::{ReadingChecker, ReadingCheckerLogic};
