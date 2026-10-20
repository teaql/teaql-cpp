use teaql_core::{SafeExpression, SmartList};
struct NoClone(i32);
fn main() {
    let expression = SafeExpression::new(NoClone(3), |root| Some(NoClone(root.0)))
        .apply(|value| NoClone(value.0 * 2))
        .apply_optional(Some);
    assert_eq!(expression.eval().unwrap().0, 6);
    assert_eq!(expression.eval_with(&NoClone(5)).unwrap().0, 10);
    assert_eq!(expression.or_else(NoClone(9)).0, 6);
    let missing = SafeExpression::new((), |_| None::<NoClone>);
    assert_eq!(missing.or_else_with(|| NoClone(11)).0, 11);
    assert_eq!(missing.or_else_throw(|| NoClone(13)).err().unwrap().0, 13);
    let identity = SafeExpression::value(7);
    assert_eq!(identity.clone().eval(), Some(7));
    assert_eq!(*identity.root(), 7);
    let list = SafeExpression::value(SmartList::new(vec![7]));
    assert_eq!(list.clone().size().eval(), Some(1));
    assert_eq!(list.clone().first().eval(), Some(7));
    assert_eq!(list.get(0).eval(), Some(7));
}
