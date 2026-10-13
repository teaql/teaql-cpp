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



    pub fn people() -> PersonRequest {
        PersonRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn people_minimal() -> PersonRequest {
        PersonRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn people_with_children() -> PersonRequest {
        PersonRequest::new()
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



    pub fn companies() -> CompanyRequest {
        CompanyRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn companies_minimal() -> CompanyRequest {
        CompanyRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn companies_with_children() -> CompanyRequest {
        CompanyRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn orders() -> OrderRequest {
        OrderRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn orders_minimal() -> OrderRequest {
        OrderRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn orders_with_children() -> OrderRequest {
        OrderRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn products() -> ProductRequest {
        ProductRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn products_minimal() -> ProductRequest {
        ProductRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn products_with_children() -> ProductRequest {
        ProductRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn lines() -> LineRequest {
        LineRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn lines_minimal() -> LineRequest {
        LineRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn lines_with_children() -> LineRequest {
        LineRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}