use teaql_core::{SafeExpression, SmartList};
struct NoClone(i32);
fn main() {
    let expression = SafeExpression::new((), |_| Some(SmartList::new(vec![NoClone(3)])));
    let _ = expression.first();
}
