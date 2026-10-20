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



    pub fn records() -> RecordRequest {
        RecordRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn records_minimal() -> RecordRequest {
        RecordRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn records_with_children() -> RecordRequest {
        RecordRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn child_records() -> ChildRecordRequest {
        ChildRecordRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn child_records_minimal() -> ChildRecordRequest {
        ChildRecordRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn child_records_with_children() -> ChildRecordRequest {
        ChildRecordRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn holders() -> HolderRequest {
        HolderRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn holders_minimal() -> HolderRequest {
        HolderRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn holders_with_children() -> HolderRequest {
        HolderRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}