use teaql_core::{SafeExpression, SmartList};
struct NoClone(i32);
fn main() {
    let expression = SafeExpression::new(3, |root| Some(NoClone(*root)));
    let _ = expression.clone();
}
