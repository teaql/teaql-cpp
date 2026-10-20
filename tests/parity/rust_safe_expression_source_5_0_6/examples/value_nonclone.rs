use teaql_core::{SafeExpression, SmartList};
struct NoClone(i32);
fn main() {
    let _ = SafeExpression::value(NoClone(3));
}
