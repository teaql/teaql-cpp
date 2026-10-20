//! Negative compiler contracts: native checked borrows adapt these static rules.
pub fn replacement_while_borrowed() {
    let mut user_context = teaql_runtime::UserContext::new();
    user_context.insert_resource(String::from("first"));
    let borrowed = user_context.require_resource::<String>().unwrap();
    user_context.insert_resource(String::from("second"));
    let _still_used = borrowed.len();
}

pub fn drop_while_borrowed() {
    let mut user_context = teaql_runtime::UserContext::new();
    user_context.insert_resource(String::from("first"));
    let borrowed = user_context.require_resource::<String>().unwrap();
    drop(user_context);
    let _still_used = borrowed.len();
}

pub fn return_borrow<'a>() -> &'a String {
    let mut user_context = teaql_runtime::UserContext::new();
    user_context.insert_resource(String::from("first"));
    user_context.require_resource::<String>().unwrap()
}
