use teaql_core::{SafeExpression, SmartList};
struct NoClone(i32);
fn main() {
    let expression = SafeExpression::new(NoClone(3), |root| Some(NoClone(root.0)));
    let _ = expression.root();
}
