
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/reading
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
#[teaql(entity = "Reading", table = "reading_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Reading {
#[teaql(id)]
    id: u64,

// @source optional_types.xml:4
#[teaql(max_length = 100)]
    text: Option<String>,

// @source optional_types.xml:4
    active: Option<bool>,

// @source optional_types.xml:4
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    balance: Option<rust_decimal::Decimal>,

// @source optional_types.xml:4
    height: Option<f64>,

// @source optional_types.xml:4
    ratio: Option<f32>,

// @source optional_types.xml:4
    count: Option<i64>,

// @source optional_types.xml:4
    day: Option<chrono::NaiveDate>,

// @source optional_types.xml:4
    recorded_at: Option<teaql_core::time::Timestamp>,

// @source optional_types.xml:4
#[teaql(large_text)]
    payload: Option<serde_json::Value>,
#[teaql(version)]
    version: i64,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Reading {
    pub const ENTITY_NAME: &'static str = "reading";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "25af758e0955ba759fb3fd64f9936595074f99b0801c334f71a5ac49420c7784";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("text", 1),
        ("active", 2),
        ("balance", 3),
        ("height", 4),
        ("ratio", 5),
        ("count", 6),
        ("day", 7),
        ("recorded_at", 8),
        ("payload", 9),
        ("version", 10),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("text", "text", "text"),
        ("active", "active", "active"),
        ("balance", "balance", "balance"),
        ("height", "height", "height"),
        ("ratio", "ratio", "ratio"),
        ("count", "count", "count"),
        ("day", "day", "day"),
        ("recorded_at", "recorded_at", "recorded_at"),
        ("payload", "payload", "payload"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            text: None,
            active: None,
            balance: None,
            height: None,
            ratio: None,
            count: None,
            day: None,
            recorded_at: None,
            payload: None,
            version: 0_i64,
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


    pub fn text(&self) -> Option<String> {
        self.text.clone()
    }

    pub fn update_text(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.text = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.text.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "text", value);
        self.__load_state.mark_loaded("text").expect("generated field layout");
        self
    }

    pub fn changed_text(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "text")
    }

    pub fn eval_text(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("text") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "text".to_string(), attempted_path: "text".to_string() }
                } else {
                    match &self.text {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn active(&self) -> Option<bool> {
        self.active.clone()
    }

    pub fn update_active(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.active = if matches!(value, teaql_core::Value::Null) { None } else { value.try_bool().map(Some).unwrap_or_else(|| self.active.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "active", value);
        self.__load_state.mark_loaded("active").expect("generated field layout");
        self
    }

    pub fn changed_active(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "active")
    }

    pub fn eval_active(&self) -> teaql_core::eval::EvalResult<Option<bool>> {
        if !self.is_loaded("active") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "active".to_string(), attempted_path: "active".to_string() }
                } else {
                    match &self.active {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn balance(&self) -> Option<rust_decimal::Decimal> {
        self.balance.clone()
    }

    pub fn update_balance(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.balance = if matches!(value, teaql_core::Value::Null) { None } else { value.try_decimal().map(Some).unwrap_or_else(|| self.balance.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "balance", value);
        self.__load_state.mark_loaded("balance").expect("generated field layout");
        self
    }

    pub fn changed_balance(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "balance")
    }

    pub fn eval_balance(&self) -> teaql_core::eval::EvalResult<Option<rust_decimal::Decimal>> {
        if !self.is_loaded("balance") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "balance".to_string(), attempted_path: "balance".to_string() }
                } else {
                    match &self.balance {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn height(&self) -> Option<f64> {
        self.height.clone()
    }

    pub fn update_height(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.height = if matches!(value, teaql_core::Value::Null) { None } else { value.try_f64().map(Some).unwrap_or_else(|| self.height.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "height", value);
        self.__load_state.mark_loaded("height").expect("generated field layout");
        self
    }

    pub fn changed_height(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "height")
    }

    pub fn eval_height(&self) -> teaql_core::eval::EvalResult<Option<f64>> {
        if !self.is_loaded("height") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "height".to_string(), attempted_path: "height".to_string() }
                } else {
                    match &self.height {
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


    pub fn count(&self) -> Option<i64> {
        self.count.clone()
    }

    pub fn update_count(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.count = if matches!(value, teaql_core::Value::Null) { None } else { value.try_i64().map(|value| value as i64).map(Some).unwrap_or_else(|| self.count.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "count", value);
        self.__load_state.mark_loaded("count").expect("generated field layout");
        self
    }

    pub fn changed_count(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "count")
    }

    pub fn eval_count(&self) -> teaql_core::eval::EvalResult<Option<i64>> {
        if !self.is_loaded("count") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "count".to_string(), attempted_path: "count".to_string() }
                } else {
                    match &self.count {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn day(&self) -> Option<chrono::NaiveDate> {
        self.day.clone()
    }

    pub fn update_day(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.day = if matches!(value, teaql_core::Value::Null) { None } else { value.try_date().map(Some).unwrap_or_else(|| self.day.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "day", value);
        self.__load_state.mark_loaded("day").expect("generated field layout");
        self
    }

    pub fn changed_day(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "day")
    }

    pub fn eval_day(&self) -> teaql_core::eval::EvalResult<Option<chrono::NaiveDate>> {
        if !self.is_loaded("day") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "day".to_string(), attempted_path: "day".to_string() }
                } else {
                    match &self.day {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn recorded_at(&self) -> Option<teaql_core::time::Timestamp> {
        self.recorded_at.clone()
    }

    pub fn update_recorded_at(&mut self, value: Option<teaql_core::time::Timestamp>) -> &mut Self {
        self.recorded_at = value;
        let value = value.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null);
        self.__teaql_runtime_state().set(self.entity_key(), "recorded_at", value);
        self.__load_state.mark_loaded("recorded_at").expect("generated field layout");
        self
    }
    pub fn changed_recorded_at(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "recorded_at")
    }

    pub fn eval_recorded_at(&self) -> teaql_core::eval::EvalResult<Option<teaql_core::time::Timestamp>> {
        if !self.is_loaded("recorded_at") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "recorded_at".to_string(), attempted_path: "recorded_at".to_string() }
                } else {
                    match &self.recorded_at {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn payload(&self) -> Option<serde_json::Value> {
        self.payload.clone()
    }

    pub fn update_payload(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.payload = if matches!(value, teaql_core::Value::Null) { None } else { Some(value.to_json_value()).map(Some).unwrap_or_else(|| self.payload.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "payload", value);
        self.__load_state.mark_loaded("payload").expect("generated field layout");
        self
    }

    pub fn changed_payload(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "payload")
    }

    pub fn eval_payload(&self) -> teaql_core::eval::EvalResult<Option<serde_json::Value>> {
        if !self.is_loaded("payload") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "payload".to_string(), attempted_path: "payload".to_string() }
                } else {
                    match &self.payload {
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


}

