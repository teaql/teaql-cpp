//! Constructor and trait boundaries of the actual generic adapter.
use std::sync::atomic::{AtomicUsize, Ordering};
use teaql_core::{CompactRow, Entity, EntityDescriptor, EntityError, MutationValues, TeaqlEntity};
use teaql_runtime::{CheckObjectStatus, CheckResults, Checker, ObjectLocation, TypedChecker,
                    TypedEntityChecker, UserContext};
static DESCRIPTORS: AtomicUsize = AtomicUsize::new(0);
struct Metadata;
impl TeaqlEntity for Metadata {
    const ENTITY_NAME: &'static str = "Metadata";
    fn entity_descriptor() -> EntityDescriptor {
        DESCRIPTORS.fetch_add(1, Ordering::Relaxed);
        EntityDescriptor::new("Metadata")
    }
}
#[derive(Clone)]
struct Model;
struct NoClone;
macro_rules! model {
    ($ty:ty, $name:literal) => {
        impl TeaqlEntity for $ty {
            const ENTITY_NAME: &'static str = $name;
            fn entity_descriptor() -> EntityDescriptor {
                DESCRIPTORS.fetch_add(1, Ordering::Relaxed);
                EntityDescriptor::new($name)
            }
        }
        impl Entity for $ty {
            fn from_compact_row(_: CompactRow) -> Result<Self, EntityError> { Ok(Self) }
            fn into_values(self) -> MutationValues { Default::default() }
        }
    };
}
model!(Model, "Entity");
model!(NoClone, "NoClone");
struct Plain;
struct Logic;
impl<T> TypedChecker<T> for Logic {
    fn check_and_fix_typed(&self, _: &UserContext, _: &mut T, _: CheckObjectStatus,
                          _: &ObjectLocation, _: &mut CheckResults) {}
}
fn require_checker(_: &impl Checker) {}
fn main() {
    assert!(std::mem::size_of::<TypedEntityChecker<(), Plain>>() > 0);
    #[cfg(feature="reject_constructor")]
    let _ = TypedEntityChecker::<(), _>::new(Plain);
    let metadata_plain = TypedEntityChecker::<Metadata, _>::new(Plain);
    let metadata_typed = TypedEntityChecker::<Metadata, _>::new(Logic);
    let entity_plain = TypedEntityChecker::<Model, _>::new(Plain);
    let nonclone = TypedEntityChecker::<NoClone, _>::new(Logic);
    let valid = TypedEntityChecker::<Model, _>::new(Logic);
    #[cfg(feature="reject_metadata")]
    require_checker(&metadata_typed);
    #[cfg(feature="reject_logic")]
    require_checker(&entity_plain);
    #[cfg(feature="reject_clone")]
    require_checker(&nonclone);
    require_checker(&valid);
    println!("opaque_alias:true\nconstructors:{}\nchecker_name:{}",
             DESCRIPTORS.load(Ordering::Relaxed), valid.entity());
    drop((metadata_plain, metadata_typed, entity_plain, nonclone));
}
