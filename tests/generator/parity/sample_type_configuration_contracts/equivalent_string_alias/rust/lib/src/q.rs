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
    pub fn priorities() -> PriorityRequest {
        PriorityRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn priorities_minimal() -> PriorityRequest {
        PriorityRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn priorities_with_children() -> PriorityRequest {
        PriorityRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn platforms() -> PlatformRequest {
        PlatformRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn platforms_minimal() -> PlatformRequest {
        PlatformRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn platforms_with_children() -> PlatformRequest {
        PlatformRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn sample_items() -> SampleItemRequest {
        SampleItemRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn sample_items_minimal() -> SampleItemRequest {
        SampleItemRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn sample_items_with_children() -> SampleItemRequest {
        SampleItemRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}