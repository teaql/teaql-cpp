
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/entry
use std::collections::BTreeMap;

use teaql_macros::{teaql_entity, TeaqlEntity};


/// [TEAQL AI WARNING]
/// TeaQL was explicitly designed to PREVENT AI hallucinations and random guessing.
/// DO NOT GUESS METHOD NAMES!
/// The methods listed below are the ONLY valid ways to interact with this entity.
/// If you encounter compilation errors (e.g., method not found), DO NOT guess another method name.
/// Read the method signatures in this file before proceeding.
#[teaql_entity]
#[derive(Clone, Debug, PartialEq, TeaqlEntity)]
#[teaql(indexed_layout)]
#[teaql(entity = "Entry", table = "entry_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Entry {
#[teaql(id)]
    id: u64,

// @source query_field_branches.xml:7
#[teaql(max_length = 100)]
    label: String,

// @source query_field_branches.xml:7
#[teaql(max_length = 100)]
    optional_label: Option<String>,

// @source query_field_branches.xml:7
#[teaql(large_text)]
    notes: Option<String>,

// @source query_field_branches.xml:7
#[teaql(max_length = 100)]
    secret: String,

// @source query_field_branches.xml:7
    active: bool,

// @source query_field_branches.xml:7
    optional_active: Option<bool>,

// @source query_field_branches.xml:7
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    amount: rust_decimal::Decimal,

// @source query_field_branches.xml:7
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    optional_amount: Option<rust_decimal::Decimal>,

// @source query_field_branches.xml:7
    height: f64,

// @source query_field_branches.xml:7
    ratio: f32,

// @source query_field_branches.xml:7
    counter: i64,

// @source query_field_branches.xml:7
    day: chrono::NaiveDate,

// @source query_field_branches.xml:7
    optional_day: Option<chrono::NaiveDate>,

// @source query_field_branches.xml:7
    moment: teaql_core::time::Timestamp,

// @source query_field_branches.xml:7
    optional_moment: Option<teaql_core::time::Timestamp>,
#[teaql(version)]
    version: i64,
// @source query_field_branches.xml:7
#[teaql(column = "tenant")]
    tenant_id: u64,
// @source query_field_branches.xml:7
#[teaql(relation(target = "Tenant", local_key = "tenant_id", foreign_key = "id"))]
    tenant: Option<Box<crate::Tenant>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Entry {
    pub const ENTITY_NAME: &'static str = "entry";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "1c010e4711eb093163582b12a727985558322bc5133b9796e8c89c96586cf4b6";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("tenant", 1),
        ("label", 2),
        ("optional_label", 3),
        ("notes", 4),
        ("secret", 5),
        ("active", 6),
        ("optional_active", 7),
        ("amount", 8),
        ("optional_amount", 9),
        ("height", 10),
        ("ratio", 11),
        ("counter", 12),
        ("day", 13),
        ("optional_day", 14),
        ("moment", 15),
        ("optional_moment", 16),
        ("version", 17),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("tenant", "tenant_id", "tenant"),
        ("label", "label", "label"),
        ("optional_label", "optional_label", "optional_label"),
        ("notes", "notes", "notes"),
        ("secret", "secret", "secret"),
        ("active", "active", "active"),
        ("optional_active", "optional_active", "optional_active"),
        ("amount", "amount", "amount"),
        ("optional_amount", "optional_amount", "optional_amount"),
        ("height", "height", "height"),
        ("ratio", "ratio", "ratio"),
        ("counter", "counter", "counter"),
        ("day", "day", "day"),
        ("optional_day", "optional_day", "optional_day"),
        ("moment", "moment", "moment"),
        ("optional_moment", "optional_moment", "optional_moment"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            label: String::new(),
            optional_label: None,
            notes: None,
            secret: String::new(),
            active: false,
            optional_active: None,
            amount: rust_decimal::Decimal::ZERO,
            optional_amount: None,
            height: 0.0_f64,
            ratio: 0.0_f32,
            counter: 0_i64,
            day: chrono::NaiveDate::from_ymd_opt(1970, 1, 1).unwrap(),
            optional_day: None,
            moment: teaql_core::time::Timestamp::now(),
            optional_moment: None,
            version: 0_i64,
            tenant_id: 0_u64,
            tenant: None,
            dynamic: BTreeMap::new(),
            __teaql_runtime_state: root,
            __load_state: teaql_core::eval::LoadState::NotLoaded.into_indexed(
                <Self as teaql_core::TeaqlEntity>::field_layout().expect("generated field layout").expect("indexed layout")
            ).expect("new entity load state"),
        }
    }

    pub fn attach_runtime_state_recursive(&mut self, root: teaql_runtime::EntityRuntimeState) {
        root.adopt_mutations_from(self.__teaql_runtime_state());
        self.__teaql_replace_runtime_state(root.clone());
        if let Some(entity) = &mut self.tenant {
            entity.attach_runtime_state_recursive(root.clone());
        }
    }

    pub fn is_loaded(&self, field_or_relation: &str) -> bool {
        self.__load_state.is_loaded(field_or_relation)
    }

    pub fn set_load_state(&mut self, state: teaql_core::eval::LoadState) {
        self.__load_state = state.into_indexed(
            <Self as teaql_core::TeaqlEntity>::field_layout().expect("generated field layout").expect("indexed layout")
        ).expect("compatible entity load state");
    }

    pub fn id(&self) -> u64 {
        self.changed_id().and_then(|value| value.try_u64()).unwrap_or(self.id)
    }

    pub fn update_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.id = value.try_u64().unwrap_or(self.id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "id", value);
        self.__load_state.mark_loaded("id").expect("generated field layout");
        self
    }

    pub fn changed_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "id")
    }

    pub fn eval_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "id".to_string(), attempted_path: "id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.id())
                }}


    pub fn label(&self) -> String {
        self.changed_label().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.label.clone())
    }

    pub fn update_label(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.label = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.label.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "label", value);
        self.__load_state.mark_loaded("label").expect("generated field layout");
        self
    }

    pub fn changed_label(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "label")
    }

    pub fn eval_label(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("label") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "label".to_string(), attempted_path: "label".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.label())
                }}


    pub fn optional_label(&self) -> Option<String> {
        self.optional_label.clone()
    }

    pub fn update_optional_label(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_label = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.optional_label.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_label", value);
        self.__load_state.mark_loaded("optional_label").expect("generated field layout");
        self
    }

    pub fn changed_optional_label(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_label")
    }

    pub fn eval_optional_label(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("optional_label") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_label".to_string(), attempted_path: "optional_label".to_string() }
                } else {
                    match &self.optional_label {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn notes(&self) -> Option<String> {
        self.notes.clone()
    }

    pub fn update_notes(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.notes = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.notes.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "notes", value);
        self.__load_state.mark_loaded("notes").expect("generated field layout");
        self
    }

    pub fn changed_notes(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "notes")
    }

    pub fn eval_notes(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("notes") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "notes".to_string(), attempted_path: "notes".to_string() }
                } else {
                    match &self.notes {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn secret(&self) -> String {
        self.changed_secret().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.secret.clone())
    }

    pub fn update_secret(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.secret = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.secret.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "secret", value);
        self.__load_state.mark_loaded("secret").expect("generated field layout");
        self
    }

    pub fn changed_secret(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "secret")
    }

    pub fn eval_secret(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("secret") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "secret".to_string(), attempted_path: "secret".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.secret())
                }}


    pub fn active(&self) -> bool {
        self.changed_active().and_then(|value| value.try_bool()).unwrap_or(self.active)
    }

    pub fn update_active(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.active = value.try_bool().unwrap_or(self.active.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "active", value);
        self.__load_state.mark_loaded("active").expect("generated field layout");
        self
    }

    pub fn changed_active(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "active")
    }

    pub fn eval_active(&self) -> teaql_core::eval::EvalResult<bool> {
        if !self.is_loaded("active") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "active".to_string(), attempted_path: "active".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.active())
                }}


    pub fn optional_active(&self) -> Option<bool> {
        self.optional_active.clone()
    }

    pub fn update_optional_active(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_active = if matches!(value, teaql_core::Value::Null) { None } else { value.try_bool().map(Some).unwrap_or_else(|| self.optional_active.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_active", value);
        self.__load_state.mark_loaded("optional_active").expect("generated field layout");
        self
    }

    pub fn changed_optional_active(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_active")
    }

    pub fn eval_optional_active(&self) -> teaql_core::eval::EvalResult<Option<bool>> {
        if !self.is_loaded("optional_active") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_active".to_string(), attempted_path: "optional_active".to_string() }
                } else {
                    match &self.optional_active {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn amount(&self) -> rust_decimal::Decimal {
        self.changed_amount().and_then(|value| value.try_decimal()).unwrap_or(self.amount)
    }

    pub fn update_amount(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.amount = value.try_decimal().unwrap_or(self.amount.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "amount", value);
        self.__load_state.mark_loaded("amount").expect("generated field layout");
        self
    }

    pub fn changed_amount(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "amount")
    }

    pub fn eval_amount(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("amount") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "amount".to_string(), attempted_path: "amount".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.amount())
                }}


    pub fn optional_amount(&self) -> Option<rust_decimal::Decimal> {
        self.optional_amount.clone()
    }

    pub fn update_optional_amount(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_amount = if matches!(value, teaql_core::Value::Null) { None } else { value.try_decimal().map(Some).unwrap_or_else(|| self.optional_amount.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_amount", value);
        self.__load_state.mark_loaded("optional_amount").expect("generated field layout");
        self
    }

    pub fn changed_optional_amount(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_amount")
    }

    pub fn eval_optional_amount(&self) -> teaql_core::eval::EvalResult<Option<rust_decimal::Decimal>> {
        if !self.is_loaded("optional_amount") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_amount".to_string(), attempted_path: "optional_amount".to_string() }
                } else {
                    match &self.optional_amount {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn height(&self) -> f64 {
        self.changed_height().and_then(|value| value.try_f64()).unwrap_or(self.height)
    }

    pub fn update_height(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.height = value.try_f64().unwrap_or(self.height.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "height", value);
        self.__load_state.mark_loaded("height").expect("generated field layout");
        self
    }

    pub fn changed_height(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "height")
    }

    pub fn eval_height(&self) -> teaql_core::eval::EvalResult<f64> {
        if !self.is_loaded("height") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "height".to_string(), attempted_path: "height".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.height())
                }}


    pub fn ratio(&self) -> f32 {
        self.changed_ratio().and_then(|value| value.try_f64()).map(|value| value as f32).unwrap_or(self.ratio)
    }

    pub fn update_ratio(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.ratio = value.try_f64().map(|value| value as f32).unwrap_or(self.ratio.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "ratio", value);
        self.__load_state.mark_loaded("ratio").expect("generated field layout");
        self
    }

    pub fn changed_ratio(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "ratio")
    }

    pub fn eval_ratio(&self) -> teaql_core::eval::EvalResult<f32> {
        if !self.is_loaded("ratio") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "ratio".to_string(), attempted_path: "ratio".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.ratio())
                }}


    pub fn counter(&self) -> i64 {
        self.changed_counter().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.counter)
    }

    pub fn update_counter(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.counter = value.try_i64().map(|value| value as i64).unwrap_or(self.counter.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "counter", value);
        self.__load_state.mark_loaded("counter").expect("generated field layout");
        self
    }

    pub fn changed_counter(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "counter")
    }

    pub fn eval_counter(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("counter") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "counter".to_string(), attempted_path: "counter".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.counter())
                }}


    pub fn day(&self) -> chrono::NaiveDate {
        self.changed_day().and_then(|value| value.try_date()).unwrap_or(self.day)
    }

    pub fn update_day(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.day = value.try_date().unwrap_or(self.day.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "day", value);
        self.__load_state.mark_loaded("day").expect("generated field layout");
        self
    }

    pub fn changed_day(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "day")
    }

    pub fn eval_day(&self) -> teaql_core::eval::EvalResult<chrono::NaiveDate> {
        if !self.is_loaded("day") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "day".to_string(), attempted_path: "day".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.day())
                }}


    pub fn optional_day(&self) -> Option<chrono::NaiveDate> {
        self.optional_day.clone()
    }

    pub fn update_optional_day(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_day = if matches!(value, teaql_core::Value::Null) { None } else { value.try_date().map(Some).unwrap_or_else(|| self.optional_day.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_day", value);
        self.__load_state.mark_loaded("optional_day").expect("generated field layout");
        self
    }

    pub fn changed_optional_day(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_day")
    }

    pub fn eval_optional_day(&self) -> teaql_core::eval::EvalResult<Option<chrono::NaiveDate>> {
        if !self.is_loaded("optional_day") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_day".to_string(), attempted_path: "optional_day".to_string() }
                } else {
                    match &self.optional_day {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn moment(&self) -> teaql_core::time::Timestamp {
        self.changed_moment().and_then(|value| value.try_timestamp()).unwrap_or(self.moment)
    }

    pub fn update_moment(&mut self, value: teaql_core::time::Timestamp) -> &mut Self {
        self.moment = value;
        let value = teaql_core::Value::from(value);
        self.__teaql_runtime_state().set(self.entity_key(), "moment", value);
        self.__load_state.mark_loaded("moment").expect("generated field layout");
        self
    }
    pub fn changed_moment(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "moment")
    }

    pub fn eval_moment(&self) -> teaql_core::eval::EvalResult<teaql_core::time::Timestamp> {
        if !self.is_loaded("moment") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "moment".to_string(), attempted_path: "moment".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.moment())
                }}


    pub fn optional_moment(&self) -> Option<teaql_core::time::Timestamp> {
        self.optional_moment.clone()
    }

    pub fn update_optional_moment(&mut self, value: Option<teaql_core::time::Timestamp>) -> &mut Self {
        self.optional_moment = value;
        let value = value.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null);
        self.__teaql_runtime_state().set(self.entity_key(), "optional_moment", value);
        self.__load_state.mark_loaded("optional_moment").expect("generated field layout");
        self
    }
    pub fn changed_optional_moment(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_moment")
    }

    pub fn eval_optional_moment(&self) -> teaql_core::eval::EvalResult<Option<teaql_core::time::Timestamp>> {
        if !self.is_loaded("optional_moment") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_moment".to_string(), attempted_path: "optional_moment".to_string() }
                } else {
                    match &self.optional_moment {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn version(&self) -> i64 {
        self.changed_version().and_then(|value| value.try_i64()).unwrap_or(self.version)
    }

    pub fn update_version(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.version = value.try_i64().unwrap_or(self.version.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "version", value);
        self.__load_state.mark_loaded("version").expect("generated field layout");
        self
    }

    pub fn changed_version(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "version")
    }

    pub fn eval_version(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("version") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "version".to_string(), attempted_path: "version".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.version())
                }}

    pub fn tenant_id(&self) -> u64 {
        self.changed_tenant_id().and_then(|value| value.try_u64()).unwrap_or(self.tenant_id)
    }

    pub(crate) fn update_tenant_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.tenant_id = value.try_u64().unwrap_or(self.tenant_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "tenant_id", value);
        self.__load_state.mark_loaded("tenant_id").expect("generated field layout");
        self
    }

    pub fn changed_tenant_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "tenant_id")
    }

    pub fn eval_tenant_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("tenant_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "tenant_id".to_string(), attempted_path: "tenant_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.tenant_id())
                }}
    pub fn tenant(&self) -> Option<&crate::Tenant> {
        self.tenant.as_deref().or_else(|| {
            if !self.is_loaded("tenant") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Tenant>(
                Self::ENTITY_NAME, self.id(), "tenant",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.tenant_id())})
    }

    pub fn eval_tenant(&self) -> teaql_core::eval::EvalResult<&crate::Tenant> {
        match self.tenant() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "tenant".to_string(), attempted_path: "tenant".to_string() },
        }
    }

}

