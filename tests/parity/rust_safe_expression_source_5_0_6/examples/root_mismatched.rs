use teaql_core::{SafeExpression, SmartList};
struct NoClone(i32);
fn main() {
    let expression = SafeExpression::new(3, |_| Some("text"));
    let _ = expression.root();
}
