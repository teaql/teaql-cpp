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



    pub fn devices() -> DeviceRequest {
        DeviceRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn devices_minimal() -> DeviceRequest {
        DeviceRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn devices_with_children() -> DeviceRequest {
        DeviceRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn order_statuses() -> OrderStatusRequest {
        OrderStatusRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn order_statuses_minimal() -> OrderStatusRequest {
        OrderStatusRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn order_statuses_with_children() -> OrderStatusRequest {
        OrderStatusRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn categories() -> CategoryRequest {
        CategoryRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn categories_minimal() -> CategoryRequest {
        CategoryRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn categories_with_children() -> CategoryRequest {
        CategoryRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn children() -> ChildRequest {
        ChildRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn children_minimal() -> ChildRequest {
        ChildRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn children_with_children() -> ChildRequest {
        ChildRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn inventory() -> InventoryRequest {
        InventoryRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn inventory_minimal() -> InventoryRequest {
        InventoryRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn inventory_with_children() -> InventoryRequest {
        InventoryRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}