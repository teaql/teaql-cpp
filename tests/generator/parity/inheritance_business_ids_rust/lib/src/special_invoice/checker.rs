
use teaql_runtime::{CheckObjectStatus, CheckResults, ObjectLocation, TypedChecker, UserContext};

pub trait SpecialInvoiceCheckerLogic: Send + Sync {
    fn check_and_fix_special_invoice(
        &self,
        _context: &UserContext,
        _entity: &mut crate::SpecialInvoice,
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
pub struct NoopSpecialInvoiceChecker;

impl SpecialInvoiceCheckerLogic for NoopSpecialInvoiceChecker {}

#[derive(Clone, Debug)]
pub struct SpecialInvoiceChecker<L = NoopSpecialInvoiceChecker> {
    logic: L,
}

impl Default for SpecialInvoiceChecker<NoopSpecialInvoiceChecker> {
    fn default() -> Self {
        Self {
            logic: NoopSpecialInvoiceChecker,
        }
    }
}

impl<L> SpecialInvoiceChecker<L>
where
    L: SpecialInvoiceCheckerLogic,
{
    pub fn new(logic: L) -> Self {
        Self { logic }
    }
}

impl<L> TypedChecker<crate::SpecialInvoice> for SpecialInvoiceChecker<L>
where
    L: SpecialInvoiceCheckerLogic,
{
    fn check_and_fix_typed(
        &self,
        context: &UserContext,
        entity: &mut crate::SpecialInvoice,
        status: CheckObjectStatus,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if status.is_create() {
            match context.require_active_root("Tenant") {
                Ok(root) if entity.tenant_id() == 0 => {
                    entity.update_tenant_id(root.id);
                    context.record_fix_evidence(teaql_runtime::FixEvidence::new("SpecialInvoice", "tenant", teaql_runtime::FixEvidenceSource::Context, "activeRoot:Tenant"));
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
            let was_missing = entity.document_number().as_str().is_empty();
            match context.require_active_root("Tenant") {
                Ok(root) => match context.business_ids() {
                    Ok(service) => {
                        let mut slot = super::entity::DocumentDocumentNumberBusinessIdSlot(entity, status.is_create());
                        match service.ensure(
                            context,
                            &crate::document::DocumentNumber::definition(),
                            &root.id.to_string(),
                            "SpecialInvoice",
                            &mut slot,
                        ) {
                            Ok(_) if was_missing => context.record_fix_evidence(teaql_runtime::FixEvidence::new(
                                    "SpecialInvoice",
                                    "document_number",
                                    teaql_runtime::FixEvidenceSource::Context,
                                    "businessIdAllocator",
                                )),
                            Ok(_) => {},
                            Err(error) => results.push(
                                teaql_runtime::CheckResult::new(
                                    teaql_runtime::CheckRule::InvalidType,
                                    location.clone().member("document_number"),
                                )
                                .with_message(error.to_string()),
                            ),
                        }
                    }
                    Err(error) => results.push(
                        teaql_runtime::CheckResult::new(
                            teaql_runtime::CheckRule::InvalidType,
                            location.clone().member("document_number"),
                        )
                        .with_message(error.to_string()),
                    ),
                },
                Err(error) => results.push(
                    teaql_runtime::CheckResult::new(
                        teaql_runtime::CheckRule::ContextRootMissing,
                        location.clone().member("document_number"),
                    )
                    .with_message(error.to_string()),
                ),
            }
        }

        if status.is_create() {
            entity.update_created_at(context.fix_time());
            context.record_fix_evidence(teaql_runtime::FixEvidence::new("SpecialInvoice", "created_at", teaql_runtime::FixEvidenceSource::Clock, "graphClock"));
        }

        if status.is_create() {
            entity.update_updated_at(context.fix_time());
            context.record_fix_evidence(teaql_runtime::FixEvidence::new("SpecialInvoice", "updated_at", teaql_runtime::FixEvidenceSource::Clock, "graphClock"));
        }
        if status.is_create() || status.is_update() {
            entity.update_updated_at(context.fix_time());
            context.record_fix_evidence(teaql_runtime::FixEvidence::new("SpecialInvoice", "updated_at", teaql_runtime::FixEvidenceSource::Clock, "graphClock"));
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

        if status.is_update() && !entity.is_loaded("tenant_id") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("tenant"),
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

        if status.is_update() && !entity.is_loaded("active") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("active"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("note") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("note"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("document_number") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("document_number"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }
        if status.is_update()
            && teaql_core::Entity::dirty_fields(entity)
                .is_some_and(|fields| fields.contains("document_number")) {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("document_number"),
                )
                .with_message("Business ID is immutable after create"),
            );
        }

        if status.is_update() && !entity.is_loaded("created_at") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("created_at"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("updated_at") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("updated_at"),
                )
                .with_message("Mutation requires a fully loaded entity"),
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

        if status.is_update() && !entity.is_loaded("amount") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("amount"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("reference") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("reference"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        self.logic
            .check_and_fix_special_invoice(context, entity, status, location, results);
    }
}