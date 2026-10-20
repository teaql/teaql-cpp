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



    pub fn json_graph_rows() -> JsonGraphRowRequest {
        JsonGraphRowRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn json_graph_rows_minimal() -> JsonGraphRowRequest {
        JsonGraphRowRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn json_graph_rows_with_children() -> JsonGraphRowRequest {
        JsonGraphRowRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn serialization_parents() -> SerializationParentRequest {
        SerializationParentRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn serialization_parents_minimal() -> SerializationParentRequest {
        SerializationParentRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn serialization_parents_with_children() -> SerializationParentRequest {
        SerializationParentRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn serialization_members() -> SerializationMemberRequest {
        SerializationMemberRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn serialization_members_minimal() -> SerializationMemberRequest {
        SerializationMemberRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn serialization_members_with_children() -> SerializationMemberRequest {
        SerializationMemberRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}