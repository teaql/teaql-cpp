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
    pub fn tenants() -> TenantRequest {
        TenantRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn tenants_minimal() -> TenantRequest {
        TenantRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn tenants_with_children() -> TenantRequest {
        TenantRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn receipts() -> ReceiptRequest {
        ReceiptRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn receipts_minimal() -> ReceiptRequest {
        ReceiptRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn receipts_with_children() -> ReceiptRequest {
        ReceiptRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }

    pub fn receipt_by_receipt_number(value: crate::ReceiptNumber) -> ReceiptRequest {
        Self::receipts()
            .with_receipt_number_is(value)
            .limit(2)
    }

    pub fn tickets() -> TicketRequest {
        TicketRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn tickets_minimal() -> TicketRequest {
        TicketRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn tickets_with_children() -> TicketRequest {
        TicketRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }

    pub fn ticket_by_ticket_number(value: crate::TicketNumber) -> TicketRequest {
        Self::tickets()
            .with_ticket_number_is(value)
            .limit(2)
    }
}