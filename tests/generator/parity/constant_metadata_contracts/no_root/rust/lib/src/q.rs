use teaql_core::Expr;

use crate::*;

pub struct PurposedQuery<T> {
    pub inner: T,
    pub purpose: String,
}

impl<T> PurposedQuery<T> {
    pub fn new(inner: T, purpose: impl Into<String>) -> Self {
        let purpose = purpose.into();
        assert!(!purpose.trim().is_empty(), "query purpose must not be empty");
        Self { inner, purpose }
    }
}

pub struct Q;

impl Q {
    pub fn statuses() -> StatusRequest {
        StatusRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn statuses_minimal() -> StatusRequest {
        StatusRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn statuses_with_children() -> StatusRequest {
        StatusRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}