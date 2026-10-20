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
    pub fn hydration_probes() -> HydrationProbeRequest {
        HydrationProbeRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn hydration_probes_minimal() -> HydrationProbeRequest {
        HydrationProbeRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn hydration_probes_with_children() -> HydrationProbeRequest {
        HydrationProbeRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}