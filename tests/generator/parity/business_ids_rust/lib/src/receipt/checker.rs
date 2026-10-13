
use teaql_runtime::{CheckObjectStatus, CheckResults, ObjectLocation, TypedChecker, UserContext};

pub trait ReceiptCheckerLogic: Send + Sync {
    fn check_and_fix_receipt(
        &self,
        _context: &UserContext,
        _entity: &mut crate::Receipt,
        _status: CheckObjectStatus,
        _location: &ObjectLocation,
        _results: &mut CheckResults,
    ) {
    }

    fn required(
        &self,
        value: bool,
        field: &str,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if !value {
            results.push(teaql_runtime::CheckResult::required(location.clone().member(field)));
        }
    }

    fn required_option<V>(
        &self,
        value: Option<&V>,
        field: &str,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if value.is_none() {
            results.push(teaql_runtime::CheckResult::required(location.clone().member(field)));
        }
    }

    fn required_text(
        &self,
        value: &str,
        field: &str,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if value.trim().is_empty() {
            results.push(teaql_runtime::CheckResult::required(location.clone().member(field)));
        }
    }

    fn min_string_length(
        &self,
        value: &str,
        field: &str,
        min_len: usize,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if value.chars().count() < min_len {
            results.push(teaql_runtime::CheckResult::min_str(
                location.clone().member(field),
                min_len as u64,
                value.to_owned(),
            ));
        }
    }

    fn max_string_length(
        &self,
        value: &str,
        field: &str,
        max_len: usize,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if value.chars().count() > max_len {
            results.push(teaql_runtime::CheckResult::max_str(
                location.clone().member(field),
                max_len as u64,
                value.to_owned(),
            ));
        }
    }
}

#[derive(Clone, Debug, Default)]
pub struct NoopReceiptChecker;

impl ReceiptCheckerLogic for NoopReceiptChecker {}

#[derive(Clone, Debug)]
pub struct ReceiptChecker<L = NoopReceiptChecker> {
    logic: L,
}

impl Default for ReceiptChecker<NoopReceiptChecker> {
    fn default() -> Self {
        Self {
            logic: NoopReceiptChecker,
        }
    }
}

impl<L> ReceiptChecker<L>
where
    L: ReceiptCheckerLogic,
{
    pub fn new(logic: L) -> Self {
        Self { logic }
    }
}

impl<L> TypedChecker<crate::Receipt> for ReceiptChecker<L>
where
    L: ReceiptCheckerLogic,
{
    fn check_and_fix_typed(
        &self,
        context: &UserContext,
        entity: &mut crate::Receipt,
        status: CheckObjectStatus,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if status.is_create() {
            match context.require_active_root("Tenant") {
                Ok(root) if entity.tenant_id() == 0 => {
                    entity.update_tenant_id(root.id);
                    context.record_fix_evidence(teaql_runtime::FixEvidence::new("Receipt", "tenant", teaql_runtime::FixEvidenceSource::Context, "activeRoot:Tenant"));
                }
                Ok(root) if entity.tenant_id() != root.id => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMismatch, location.clone().member("tenant"))),
                Ok(_) => {}
                Err(error) => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMissing, location.clone().member("tenant")).with_message(error.to_string())),
            }
        } else if status.is_update() && entity.tenant_id() != 0 {
            match context.require_active_root("Tenant") {
                Ok(root) if entity.tenant_id() != root.id => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMismatch, location.clone().member("tenant"))),
                Ok(_) => {}
                Err(error) => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMissing, location.clone().member("tenant")).with_message(error.to_string())),
            }
        }

        if status.is_create() {
            let was_missing = entity.receipt_number().as_str().is_empty();
            match context.require_active_root("Tenant") {
                Ok(root) => match context.business_ids() {
                    Ok(service) => {
                        let mut slot = super::entity::ReceiptReceiptNumberBusinessIdSlot(entity, status.is_create());
                        match service.ensure(
                            context,
                            &crate::receipt::ReceiptNumber::definition(),
                            &root.id.to_string(),
                            "Receipt",
                            &mut slot,
                        ) {
                            Ok(_) if was_missing => context.record_fix_evidence(teaql_runtime::FixEvidence::new(
                                    "Receipt",
                                    "receipt_number",
                                    teaql_runtime::FixEvidenceSource::Context,
                                    "businessIdAllocator",
                                )),
                            Ok(_) => {},
                            Err(error) => results.push(
                                teaql_runtime::CheckResult::new(
                                    teaql_runtime::CheckRule::InvalidType,
                                    location.clone().member("receipt_number"),
                                )
                                .with_message(error.to_string()),
                            ),
                        }
                    }
                    Err(error) => results.push(
                        teaql_runtime::CheckResult::new(
                            teaql_runtime::CheckRule::InvalidType,
                            location.clone().member("receipt_number"),
                        )
                        .with_message(error.to_string()),
                    ),
                },
                Err(error) => results.push(
                    teaql_runtime::CheckResult::new(
                        teaql_runtime::CheckRule::ContextRootMissing,
                        location.clone().member("receipt_number"),
                    )
                    .with_message(error.to_string()),
                ),
            }
        }


        if status.is_update() && !entity.is_loaded("id") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("id"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("name") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("name"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("tenant_id") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("tenant"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("receipt_number") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("receipt_number"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }
        if status.is_update()
            && teaql_core::Entity::dirty_fields(entity)
                .is_some_and(|fields| fields.contains("receipt_number")) {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("receipt_number"),
                )
                .with_message("Business ID is immutable after create"),
            );
        }

        if status.is_update() && !entity.is_loaded("version") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("version"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        self.logic
            .check_and_fix_receipt(context, entity, status, location, results);
    }
}