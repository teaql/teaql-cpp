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



    pub fn owners() -> OwnerRequest {
        OwnerRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn owners_minimal() -> OwnerRequest {
        OwnerRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn owners_with_children() -> OwnerRequest {
        OwnerRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn entries() -> EntryRequest {
        EntryRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn entries_minimal() -> EntryRequest {
        EntryRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn entries_with_children() -> EntryRequest {
        EntryRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn profiles() -> ProfileRequest {
        ProfileRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn profiles_minimal() -> ProfileRequest {
        ProfileRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn profiles_with_children() -> ProfileRequest {
        ProfileRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}