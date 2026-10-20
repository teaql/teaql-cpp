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



    pub fn classes() -> ClassRequest {
        ClassRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn classes_minimal() -> ClassRequest {
        ClassRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn classes_with_children() -> ClassRequest {
        ClassRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn operators() -> OperatorRequest {
        OperatorRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn operators_minimal() -> OperatorRequest {
        OperatorRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn operators_with_children() -> OperatorRequest {
        OperatorRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn templates() -> TemplateRequest {
        TemplateRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn templates_minimal() -> TemplateRequest {
        TemplateRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn templates_with_children() -> TemplateRequest {
        TemplateRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn namespaces() -> NamespaceRequest {
        NamespaceRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn namespaces_minimal() -> NamespaceRequest {
        NamespaceRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn namespaces_with_children() -> NamespaceRequest {
        NamespaceRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn plain_records() -> PlainRecordRequest {
        PlainRecordRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn plain_records_minimal() -> PlainRecordRequest {
        PlainRecordRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn plain_records_with_children() -> PlainRecordRequest {
        PlainRecordRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}