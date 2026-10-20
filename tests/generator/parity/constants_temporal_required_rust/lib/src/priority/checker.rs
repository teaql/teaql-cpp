
use teaql_runtime::{CheckObjectStatus, CheckResults, ObjectLocation, TypedChecker, UserContext};

pub trait PriorityCheckerLogic: Send + Sync {
    fn check_and_fix_priority(
        &self,
        _context: &UserContext,
        _entity: &mut crate::Priority,
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
pub struct NoopPriorityChecker;

impl PriorityCheckerLogic for NoopPriorityChecker {}

#[derive(Clone, Debug)]
pub struct PriorityChecker<L = NoopPriorityChecker> {
    logic: L,
}

impl Default for PriorityChecker<NoopPriorityChecker> {
    fn default() -> Self {
        Self {
            logic: NoopPriorityChecker,
        }
    }
}

impl<L> PriorityChecker<L>
where
    L: PriorityCheckerLogic,
{
    pub fn new(logic: L) -> Self {
        Self { logic }
    }
}

impl<L> TypedChecker<crate::Priority> for PriorityChecker<L>
where
    L: PriorityCheckerLogic,
{
    fn check_and_fix_typed(
        &self,
        context: &UserContext,
        entity: &mut crate::Priority,
        status: CheckObjectStatus,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        if status.is_create() {
            match context.require_active_root("Platform") {
                Ok(root) if entity.platform_id() == 0 => {
                    entity.update_platform_id(root.id);
                    context.record_fix_evidence(teaql_runtime::FixEvidence::new("Priority", "platform", teaql_runtime::FixEvidenceSource::Context, "activeRoot:Platform"));
                }
                Ok(root) if entity.platform_id() != root.id => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMismatch, location.clone().member("platform"))),
                Ok(_) => {}
                Err(error) => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMissing, location.clone().member("platform")).with_message(error.to_string())),
            }
        } else if status.is_update() && entity.platform_id() != 0 {
            match context.require_active_root("Platform") {
                Ok(root) if entity.platform_id() != root.id => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMismatch, location.clone().member("platform"))),
                Ok(_) => {}
                Err(error) => results.push(teaql_runtime::CheckResult::new(teaql_runtime::CheckRule::ContextRootMissing, location.clone().member("platform")).with_message(error.to_string())),
            }
        }






        if status.is_create() {
            entity.update_created_at(context.fix_time());
            context.record_fix_evidence(teaql_runtime::FixEvidence::new("Priority", "created_at", teaql_runtime::FixEvidenceSource::Clock, "graphClock"));
        }

        if status.is_create() {
            entity.update_updated_at(context.fix_time());
            context.record_fix_evidence(teaql_runtime::FixEvidence::new("Priority", "updated_at", teaql_runtime::FixEvidenceSource::Clock, "graphClock"));
        }
        if status.is_create() || status.is_update() {
            entity.update_updated_at(context.fix_time());
            context.record_fix_evidence(teaql_runtime::FixEvidence::new("Priority", "updated_at", teaql_runtime::FixEvidenceSource::Clock, "graphClock"));
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

        if status.is_update() && !entity.is_loaded("platform_id") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("platform"),
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

        if status.is_update() && !entity.is_loaded("code") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("code"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("instant") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("instant"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("clock") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("clock"),
                )
                .with_message("Mutation requires a fully loaded entity"),
            );
        }

        if status.is_update() && !entity.is_loaded("optional_clock") {
            results.push(
                teaql_runtime::CheckResult::new(
                    teaql_runtime::CheckRule::InvalidType,
                    location.clone().member("optional_clock"),
                )
                .with_message("Mutation requires a fully loaded entity"),
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

        self.logic
            .check_and_fix_priority(context, entity, status, location, results);
    }
}