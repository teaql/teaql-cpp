
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/sample_record
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
#[teaql(reverse_relation(name = "first_links", target = "SampleLink", json_type = "crate::SampleLink", local_key = "id", foreign_key = "first_record_id", many))]
#[teaql(reverse_relation(name = "second_links", target = "SampleLink", json_type = "crate::SampleLink", local_key = "id", foreign_key = "second_record_id", many))]
#[teaql(entity = "SampleRecord", table = "sample_record_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct SampleRecord {
#[teaql(id)]
    id: u64,

// @source sample_fields_temporal.xml:10
#[teaql(max_length = 100)]
    label: String,

// @source sample_fields_temporal.xml:10
    quantity: i64,

// @source sample_fields_temporal.xml:10
    zero_bound: i64,

// @source sample_fields_temporal.xml:10
    negative_max: i64,

// @source sample_fields_temporal.xml:10
    signed_max: i64,

// @source sample_fields_temporal.xml:10
    wide_max: i64,

// @source sample_fields_temporal.xml:10
#[teaql(max_length = 100)]
    negative_bound: String,

// @source sample_fields_temporal.xml:10
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    amount: rust_decimal::Decimal,

// @source sample_fields_temporal.xml:10
    height: f64,

// @source sample_fields_temporal.xml:10
    optional_count: Option<i64>,

// @source sample_fields_temporal.xml:10
    active: bool,

// @source sample_fields_temporal.xml:10
    disabled: bool,

// @source sample_fields_temporal.xml:10
    optional_active: Option<bool>,

// @source sample_fields_temporal.xml:10
    day: chrono::NaiveDate,

// @source sample_fields_temporal.xml:10
    optional_day: Option<chrono::NaiveDate>,

// @source sample_fields_temporal.xml:10
    recorded_at: teaql_core::time::Timestamp,

// @source sample_fields_temporal.xml:10
    optional_recorded_at: Option<teaql_core::time::Timestamp>,

// @source sample_fields_temporal.xml:10
    clock_text: String,

// @source sample_fields_temporal.xml:10
    optional_clock_text: Option<String>,

// @source sample_fields_temporal.xml:10
    ratio: Option<f32>,
#[teaql(version)]
    version: i64,
// @source sample_fields_temporal.xml:10
#[teaql(column = "platform")]
    platform_id: u64,
// @source sample_fields_temporal.xml:10
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl SampleRecord {
    pub const ENTITY_NAME: &'static str = "sample_record";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "249f72ffb6af59610734469efa1c33f71075b7258858050e65a3535d45a29ab6";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("label", 2),
        ("quantity", 3),
        ("zero_bound", 4),
        ("negative_max", 5),
        ("signed_max", 6),
        ("wide_max", 7),
        ("negative_bound", 8),
        ("amount", 9),
        ("height", 10),
        ("optional_count", 11),
        ("active", 12),
        ("disabled", 13),
        ("optional_active", 14),
        ("day", 15),
        ("optional_day", 16),
        ("recorded_at", 17),
        ("optional_recorded_at", 18),
        ("clock_text", 19),
        ("optional_clock_text", 20),
        ("ratio", 21),
        ("version", 22),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("label", "label", "label"),
        ("quantity", "quantity", "quantity"),
        ("zero_bound", "zero_bound", "zero_bound"),
        ("negative_max", "negative_max", "negative_max"),
        ("signed_max", "signed_max", "signed_max"),
        ("wide_max", "wide_max", "wide_max"),
        ("negative_bound", "negative_bound", "negative_bound"),
        ("amount", "amount", "amount"),
        ("height", "height", "height"),
        ("optional_count", "optional_count", "optional_count"),
        ("active", "active", "active"),
        ("disabled", "disabled", "disabled"),
        ("optional_active", "optional_active", "optional_active"),
        ("day", "day", "day"),
        ("optional_day", "optional_day", "optional_day"),
        ("recorded_at", "recorded_at", "recorded_at"),
        ("optional_recorded_at", "optional_recorded_at", "optional_recorded_at"),
        ("clock_text", "clock_text", "clock_text"),
        ("optional_clock_text", "optional_clock_text", "optional_clock_text"),
        ("ratio", "ratio", "ratio"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            label: String::new(),
            quantity: 0_i64,
            zero_bound: 0_i64,
            negative_max: 0_i64,
            signed_max: 0_i64,
            wide_max: 0_i64,
            negative_bound: String::new(),
            amount: rust_decimal::Decimal::ZERO,
            height: 0.0_f64,
            optional_count: None,
            active: false,
            disabled: false,
            optional_active: None,
            day: chrono::NaiveDate::from_ymd_opt(1970, 1, 1).unwrap(),
            optional_day: None,
            recorded_at: teaql_core::time::Timestamp::now(),
            optional_recorded_at: None,
            clock_text: String::new(),
            optional_clock_text: None,
            ratio: None,
            version: 0_i64,
            platform_id: 0_u64,
            platform: None,
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
        if let Some(entity) = &mut self.platform {
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


    pub fn quantity(&self) -> i64 {
        self.changed_quantity().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.quantity)
    }

    pub fn update_quantity(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.quantity = value.try_i64().map(|value| value as i64).unwrap_or(self.quantity.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "quantity", value);
        self.__load_state.mark_loaded("quantity").expect("generated field layout");
        self
    }

    pub fn changed_quantity(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "quantity")
    }

    pub fn eval_quantity(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("quantity") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "quantity".to_string(), attempted_path: "quantity".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.quantity())
                }}


    pub fn zero_bound(&self) -> i64 {
        self.changed_zero_bound().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.zero_bound)
    }

    pub fn update_zero_bound(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.zero_bound = value.try_i64().map(|value| value as i64).unwrap_or(self.zero_bound.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "zero_bound", value);
        self.__load_state.mark_loaded("zero_bound").expect("generated field layout");
        self
    }

    pub fn changed_zero_bound(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "zero_bound")
    }

    pub fn eval_zero_bound(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("zero_bound") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "zero_bound".to_string(), attempted_path: "zero_bound".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.zero_bound())
                }}


    pub fn negative_max(&self) -> i64 {
        self.changed_negative_max().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.negative_max)
    }

    pub fn update_negative_max(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.negative_max = value.try_i64().map(|value| value as i64).unwrap_or(self.negative_max.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "negative_max", value);
        self.__load_state.mark_loaded("negative_max").expect("generated field layout");
        self
    }

    pub fn changed_negative_max(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "negative_max")
    }

    pub fn eval_negative_max(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("negative_max") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "negative_max".to_string(), attempted_path: "negative_max".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.negative_max())
                }}


    pub fn signed_max(&self) -> i64 {
        self.changed_signed_max().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.signed_max)
    }

    pub fn update_signed_max(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.signed_max = value.try_i64().map(|value| value as i64).unwrap_or(self.signed_max.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "signed_max", value);
        self.__load_state.mark_loaded("signed_max").expect("generated field layout");
        self
    }

    pub fn changed_signed_max(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "signed_max")
    }

    pub fn eval_signed_max(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("signed_max") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "signed_max".to_string(), attempted_path: "signed_max".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.signed_max())
                }}


    pub fn wide_max(&self) -> i64 {
        self.changed_wide_max().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.wide_max)
    }

    pub fn update_wide_max(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.wide_max = value.try_i64().map(|value| value as i64).unwrap_or(self.wide_max.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "wide_max", value);
        self.__load_state.mark_loaded("wide_max").expect("generated field layout");
        self
    }

    pub fn changed_wide_max(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "wide_max")
    }

    pub fn eval_wide_max(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("wide_max") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "wide_max".to_string(), attempted_path: "wide_max".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.wide_max())
                }}


    pub fn negative_bound(&self) -> String {
        self.changed_negative_bound().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.negative_bound.clone())
    }

    pub fn update_negative_bound(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.negative_bound = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.negative_bound.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "negative_bound", value);
        self.__load_state.mark_loaded("negative_bound").expect("generated field layout");
        self
    }

    pub fn changed_negative_bound(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "negative_bound")
    }

    pub fn eval_negative_bound(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("negative_bound") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "negative_bound".to_string(), attempted_path: "negative_bound".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.negative_bound())
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


    pub fn optional_count(&self) -> Option<i64> {
        self.optional_count.clone()
    }

    pub fn update_optional_count(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_count = if matches!(value, teaql_core::Value::Null) { None } else { value.try_i64().map(|value| value as i64).map(Some).unwrap_or_else(|| self.optional_count.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_count", value);
        self.__load_state.mark_loaded("optional_count").expect("generated field layout");
        self
    }

    pub fn changed_optional_count(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_count")
    }

    pub fn eval_optional_count(&self) -> teaql_core::eval::EvalResult<Option<i64>> {
        if !self.is_loaded("optional_count") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_count".to_string(), attempted_path: "optional_count".to_string() }
                } else {
                    match &self.optional_count {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
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


    pub fn disabled(&self) -> bool {
        self.changed_disabled().and_then(|value| value.try_bool()).unwrap_or(self.disabled)
    }

    pub fn update_disabled(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.disabled = value.try_bool().unwrap_or(self.disabled.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "disabled", value);
        self.__load_state.mark_loaded("disabled").expect("generated field layout");
        self
    }

    pub fn changed_disabled(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "disabled")
    }

    pub fn eval_disabled(&self) -> teaql_core::eval::EvalResult<bool> {
        if !self.is_loaded("disabled") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "disabled".to_string(), attempted_path: "disabled".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.disabled())
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


    pub fn recorded_at(&self) -> teaql_core::time::Timestamp {
        self.changed_recorded_at().and_then(|value| value.try_timestamp()).unwrap_or(self.recorded_at)
    }

    pub fn update_recorded_at(&mut self, value: teaql_core::time::Timestamp) -> &mut Self {
        self.recorded_at = value;
        let value = teaql_core::Value::from(value);
        self.__teaql_runtime_state().set(self.entity_key(), "recorded_at", value);
        self.__load_state.mark_loaded("recorded_at").expect("generated field layout");
        self
    }
    pub fn changed_recorded_at(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "recorded_at")
    }

    pub fn eval_recorded_at(&self) -> teaql_core::eval::EvalResult<teaql_core::time::Timestamp> {
        if !self.is_loaded("recorded_at") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "recorded_at".to_string(), attempted_path: "recorded_at".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.recorded_at())
                }}


    pub fn optional_recorded_at(&self) -> Option<teaql_core::time::Timestamp> {
        self.optional_recorded_at.clone()
    }

    pub fn update_optional_recorded_at(&mut self, value: Option<teaql_core::time::Timestamp>) -> &mut Self {
        self.optional_recorded_at = value;
        let value = value.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null);
        self.__teaql_runtime_state().set(self.entity_key(), "optional_recorded_at", value);
        self.__load_state.mark_loaded("optional_recorded_at").expect("generated field layout");
        self
    }
    pub fn changed_optional_recorded_at(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_recorded_at")
    }

    pub fn eval_optional_recorded_at(&self) -> teaql_core::eval::EvalResult<Option<teaql_core::time::Timestamp>> {
        if !self.is_loaded("optional_recorded_at") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_recorded_at".to_string(), attempted_path: "optional_recorded_at".to_string() }
                } else {
                    match &self.optional_recorded_at {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn clock_text(&self) -> String {
        self.changed_clock_text().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.clock_text.clone())
    }

    pub fn update_clock_text(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.clock_text = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.clock_text.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "clock_text", value);
        self.__load_state.mark_loaded("clock_text").expect("generated field layout");
        self
    }

    pub fn changed_clock_text(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "clock_text")
    }

    pub fn eval_clock_text(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("clock_text") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "clock_text".to_string(), attempted_path: "clock_text".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.clock_text())
                }}


    pub fn optional_clock_text(&self) -> Option<String> {
        self.optional_clock_text.clone()
    }

    pub fn update_optional_clock_text(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_clock_text = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.optional_clock_text.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_clock_text", value);
        self.__load_state.mark_loaded("optional_clock_text").expect("generated field layout");
        self
    }

    pub fn changed_optional_clock_text(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_clock_text")
    }

    pub fn eval_optional_clock_text(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("optional_clock_text") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_clock_text".to_string(), attempted_path: "optional_clock_text".to_string() }
                } else {
                    match &self.optional_clock_text {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn ratio(&self) -> Option<f32> {
        self.ratio.clone()
    }

    pub fn update_ratio(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.ratio = if matches!(value, teaql_core::Value::Null) { None } else { value.try_f64().map(|value| value as f32).map(Some).unwrap_or_else(|| self.ratio.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "ratio", value);
        self.__load_state.mark_loaded("ratio").expect("generated field layout");
        self
    }

    pub fn changed_ratio(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "ratio")
    }

    pub fn eval_ratio(&self) -> teaql_core::eval::EvalResult<Option<f32>> {
        if !self.is_loaded("ratio") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "ratio".to_string(), attempted_path: "ratio".to_string() }
                } else {
                    match &self.ratio {
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

    pub fn platform_id(&self) -> u64 {
        self.changed_platform_id().and_then(|value| value.try_u64()).unwrap_or(self.platform_id)
    }

    pub fn update_platform_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.platform_id = value.try_u64().unwrap_or(self.platform_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "platform_id", value);
        self.__load_state.mark_loaded("platform_id").expect("generated field layout");
        self
    }

    pub fn changed_platform_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "platform_id")
    }

    pub fn eval_platform_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("platform_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "platform_id".to_string(), attempted_path: "platform_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.platform_id())
                }}
    pub fn platform(&self) -> Option<&crate::Platform> {
        self.platform.as_deref().or_else(|| {
            if !self.is_loaded("platform") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Platform>(
                Self::ENTITY_NAME, self.id(), "platform",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.platform_id())})
    }

    pub fn eval_platform(&self) -> teaql_core::eval::EvalResult<&crate::Platform> {
        match self.platform() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "platform".to_string(), attempted_path: "platform".to_string() },
        }
    }
    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn first_links(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::SampleLink>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "first_links",
        )
    }

    pub fn eval_first_links(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::SampleLink>> {
        let relation = self.first_links();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "first_links".to_string(), attempted_path: "first_links".to_string() },
        }
    }

    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn second_links(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::SampleLink>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "second_links",
        )
    }

    pub fn eval_second_links(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::SampleLink>> {
        let relation = self.second_links();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "second_links".to_string(), attempted_path: "second_links".to_string() },
        }
    }

}

