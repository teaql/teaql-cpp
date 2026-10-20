use std::collections::BTreeMap;
use teaql_core::{MutationValues, GeneratedValues, EntitySnapshot, MutationKind, Value};
fn roles<T: Clone + Default + PartialEq + std::fmt::Debug>() {}
fn copy_eq<T: Copy + Eq>() {}
fn main() {
    roles::<MutationValues>();
    roles::<GeneratedValues>();
    roles::<EntitySnapshot>();
    copy_eq::<MutationKind>();
    let mut input = MutationValues::new();
    input.insert("b".into(), Value::U64(7));
    input.insert("a".into(), Value::Null);
    let order: Vec<_> = (&input).into_iter().map(|(name, _)| name.clone()).collect();
    assert_eq!(order, vec!["a", "b"]);
    let owned: Vec<_> = input.clone().into_iter().collect();
    assert_eq!(owned.len(), 2);
    let record: BTreeMap<_, _> = input.into();
    let generated = GeneratedValues::from(record);
    let record: BTreeMap<_, _> = generated.into();
    let snapshot = EntitySnapshot::from(record);
    assert_eq!(snapshot.len(), 2);
    let record: BTreeMap<_, _> = snapshot.into();
    assert_eq!(MutationValues::from(record).len(), 2);
}
