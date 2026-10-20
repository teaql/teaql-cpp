
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/note
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
#[teaql(entity = "Note", table = "note_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Note {
#[teaql(id)]
    id: u64,

// @source checker_clock_scalars.xml:14
#[teaql(max_length = 100)]
    text: String,

// @source checker_clock_scalars.xml:14
#[teaql(max_length = 100)]
    optional_text: Option<String>,

// @source checker_clock_scalars.xml:14
#[teaql(max_length = 100)]
    code: String,

// @source checker_clock_scalars.xml:14
#[teaql(max_length = 100)]
    optional_code: Option<String>,

// @source checker_clock_scalars.xml:14
    integer: i64,

// @source checker_clock_scalars.xml:14
    optional_integer: Option<i64>,

// @source checker_clock_scalars.xml:14
    long: i64,

// @source checker_clock_scalars.xml:14
    optional_long: Option<i64>,

// @source checker_clock_scalars.xml:14
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    decimal: rust_decimal::Decimal,

// @source checker_clock_scalars.xml:14
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    optional_decimal: Option<rust_decimal::Decimal>,

// @source checker_clock_scalars.xml:14
    float: f32,

// @source checker_clock_scalars.xml:14
    optional_float: Option<f32>,

// @source checker_clock_scalars.xml:14
    double: f64,

// @source checker_clock_scalars.xml:14
    optional_double: Option<f64>,

// @source checker_clock_scalars.xml:14
    active: bool,

// @source checker_clock_scalars.xml:14
    optional_active: Option<bool>,

// @source checker_clock_scalars.xml:14
    date: chrono::NaiveDate,

// @source checker_clock_scalars.xml:14
    optional_date: Option<chrono::NaiveDate>,

// @source checker_clock_scalars.xml:14
    time: String,

// @source checker_clock_scalars.xml:14
    optional_time: Option<String>,

// @source checker_clock_scalars.xml:14
#[teaql(large_text)]
    payload: Option<serde_json::Value>,

// @source checker_clock_scalars.xml:14
#[teaql(large_text)]
    optional_payload: Option<serde_json::Value>,

// @source checker_clock_scalars.xml:14
    timestamp: teaql_core::time::Timestamp,
#[teaql(version)]
    version: i64,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Note {
    pub const ENTITY_NAME: &'static str = "note";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "751bc440f1637fdcb269ee2d2e9592a22ec862b78473655b609a1225506974fe";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("text", 1),
        ("optional_text", 2),
        ("code", 3),
        ("optional_code", 4),
        ("integer", 5),
        ("optional_integer", 6),
        ("long", 7),
        ("optional_long", 8),
        ("decimal", 9),
        ("optional_decimal", 10),
        ("float", 11),
        ("optional_float", 12),
        ("double", 13),
        ("optional_double", 14),
        ("active", 15),
        ("optional_active", 16),
        ("date", 17),
        ("optional_date", 18),
        ("time", 19),
        ("optional_time", 20),
        ("payload", 21),
        ("optional_payload", 22),
        ("timestamp", 23),
        ("version", 24),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("text", "text", "text"),
        ("optional_text", "optional_text", "optional_text"),
        ("code", "code", "code"),
        ("optional_code", "optional_code", "optional_code"),
        ("integer", "integer", "integer"),
        ("optional_integer", "optional_integer", "optional_integer"),
        ("long", "long", "long"),
        ("optional_long", "optional_long", "optional_long"),
        ("decimal", "decimal", "decimal"),
        ("optional_decimal", "optional_decimal", "optional_decimal"),
        ("float", "float", "float"),
        ("optional_float", "optional_float", "optional_float"),
        ("double", "double", "double"),
        ("optional_double", "optional_double", "optional_double"),
        ("active", "active", "active"),
        ("optional_active", "optional_active", "optional_active"),
        ("date", "date", "date"),
        ("optional_date", "optional_date", "optional_date"),
        ("time", "time", "time"),
        ("optional_time", "optional_time", "optional_time"),
        ("payload", "payload", "payload"),
        ("optional_payload", "optional_payload", "optional_payload"),
        ("timestamp", "timestamp", "timestamp"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            text: String::new(),
            optional_text: None,
            code: String::new(),
            optional_code: None,
            integer: 0_i64,
            optional_integer: None,
            long: 0_i64,
            optional_long: None,
            decimal: rust_decimal::Decimal::ZERO,
            optional_decimal: None,
            float: 0.0_f32,
            optional_float: None,
            double: 0.0_f64,
            optional_double: None,
            active: false,
            optional_active: None,
            date: chrono::NaiveDate::from_ymd_opt(1970, 1, 1).unwrap(),
            optional_date: None,
            time: String::new(),
            optional_time: None,
            payload: None,
            optional_payload: None,
            timestamp: teaql_core::time::Timestamp::now(),
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


    pub fn text(&self) -> String {
        self.changed_text().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.text.clone())
    }

    pub fn update_text(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.text = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.text.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "text", value);
        self.__load_state.mark_loaded("text").expect("generated field layout");
        self
    }

    pub fn changed_text(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "text")
    }

    pub fn eval_text(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("text") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "text".to_string(), attempted_path: "text".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.text())
                }}


    pub fn optional_text(&self) -> Option<String> {
        self.optional_text.clone()
    }

    pub fn update_optional_text(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_text = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.optional_text.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_text", value);
        self.__load_state.mark_loaded("optional_text").expect("generated field layout");
        self
    }

    pub fn changed_optional_text(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_text")
    }

    pub fn eval_optional_text(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("optional_text") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_text".to_string(), attempted_path: "optional_text".to_string() }
                } else {
                    match &self.optional_text {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn code(&self) -> String {
        self.changed_code().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.code.clone())
    }

    pub fn update_code(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.code = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.code.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "code", value);
        self.__load_state.mark_loaded("code").expect("generated field layout");
        self
    }

    pub fn changed_code(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "code")
    }

    pub fn eval_code(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("code") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "code".to_string(), attempted_path: "code".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.code())
                }}


    pub fn optional_code(&self) -> Option<String> {
        self.optional_code.clone()
    }

    pub fn update_optional_code(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_code = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.optional_code.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_code", value);
        self.__load_state.mark_loaded("optional_code").expect("generated field layout");
        self
    }

    pub fn changed_optional_code(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_code")
    }

    pub fn eval_optional_code(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("optional_code") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_code".to_string(), attempted_path: "optional_code".to_string() }
                } else {
                    match &self.optional_code {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn integer(&self) -> i64 {
        self.changed_integer().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.integer)
    }

    pub fn update_integer(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.integer = value.try_i64().map(|value| value as i64).unwrap_or(self.integer.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "integer", value);
        self.__load_state.mark_loaded("integer").expect("generated field layout");
        self
    }

    pub fn changed_integer(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "integer")
    }

    pub fn eval_integer(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("integer") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "integer".to_string(), attempted_path: "integer".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.integer())
                }}


    pub fn optional_integer(&self) -> Option<i64> {
        self.optional_integer.clone()
    }

    pub fn update_optional_integer(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_integer = if matches!(value, teaql_core::Value::Null) { None } else { value.try_i64().map(|value| value as i64).map(Some).unwrap_or_else(|| self.optional_integer.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_integer", value);
        self.__load_state.mark_loaded("optional_integer").expect("generated field layout");
        self
    }

    pub fn changed_optional_integer(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_integer")
    }

    pub fn eval_optional_integer(&self) -> teaql_core::eval::EvalResult<Option<i64>> {
        if !self.is_loaded("optional_integer") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_integer".to_string(), attempted_path: "optional_integer".to_string() }
                } else {
                    match &self.optional_integer {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn long(&self) -> i64 {
        self.changed_long().and_then(|value| value.try_i64()).unwrap_or(self.long)
    }

    pub fn update_long(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.long = value.try_i64().unwrap_or(self.long.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "long", value);
        self.__load_state.mark_loaded("long").expect("generated field layout");
        self
    }

    pub fn changed_long(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "long")
    }

    pub fn eval_long(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("long") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "long".to_string(), attempted_path: "long".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.long())
                }}


    pub fn optional_long(&self) -> Option<i64> {
        self.optional_long.clone()
    }

    pub fn update_optional_long(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_long = if matches!(value, teaql_core::Value::Null) { None } else { value.try_i64().map(Some).unwrap_or_else(|| self.optional_long.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_long", value);
        self.__load_state.mark_loaded("optional_long").expect("generated field layout");
        self
    }

    pub fn changed_optional_long(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_long")
    }

    pub fn eval_optional_long(&self) -> teaql_core::eval::EvalResult<Option<i64>> {
        if !self.is_loaded("optional_long") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_long".to_string(), attempted_path: "optional_long".to_string() }
                } else {
                    match &self.optional_long {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn decimal(&self) -> rust_decimal::Decimal {
        self.changed_decimal().and_then(|value| value.try_decimal()).unwrap_or(self.decimal)
    }

    pub fn update_decimal(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.decimal = value.try_decimal().unwrap_or(self.decimal.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "decimal", value);
        self.__load_state.mark_loaded("decimal").expect("generated field layout");
        self
    }

    pub fn changed_decimal(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "decimal")
    }

    pub fn eval_decimal(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("decimal") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "decimal".to_string(), attempted_path: "decimal".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.decimal())
                }}


    pub fn optional_decimal(&self) -> Option<rust_decimal::Decimal> {
        self.optional_decimal.clone()
    }

    pub fn update_optional_decimal(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_decimal = if matches!(value, teaql_core::Value::Null) { None } else { value.try_decimal().map(Some).unwrap_or_else(|| self.optional_decimal.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_decimal", value);
        self.__load_state.mark_loaded("optional_decimal").expect("generated field layout");
        self
    }

    pub fn changed_optional_decimal(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_decimal")
    }

    pub fn eval_optional_decimal(&self) -> teaql_core::eval::EvalResult<Option<rust_decimal::Decimal>> {
        if !self.is_loaded("optional_decimal") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_decimal".to_string(), attempted_path: "optional_decimal".to_string() }
                } else {
                    match &self.optional_decimal {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn float(&self) -> f32 {
        self.changed_float().and_then(|value| value.try_f64()).map(|value| value as f32).unwrap_or(self.float)
    }

    pub fn update_float(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.float = value.try_f64().map(|value| value as f32).unwrap_or(self.float.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "float", value);
        self.__load_state.mark_loaded("float").expect("generated field layout");
        self
    }

    pub fn changed_float(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "float")
    }

    pub fn eval_float(&self) -> teaql_core::eval::EvalResult<f32> {
        if !self.is_loaded("float") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "float".to_string(), attempted_path: "float".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.float())
                }}


    pub fn optional_float(&self) -> Option<f32> {
        self.optional_float.clone()
    }

    pub fn update_optional_float(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_float = if matches!(value, teaql_core::Value::Null) { None } else { value.try_f64().map(|value| value as f32).map(Some).unwrap_or_else(|| self.optional_float.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_float", value);
        self.__load_state.mark_loaded("optional_float").expect("generated field layout");
        self
    }

    pub fn changed_optional_float(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_float")
    }

    pub fn eval_optional_float(&self) -> teaql_core::eval::EvalResult<Option<f32>> {
        if !self.is_loaded("optional_float") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_float".to_string(), attempted_path: "optional_float".to_string() }
                } else {
                    match &self.optional_float {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn double(&self) -> f64 {
        self.changed_double().and_then(|value| value.try_f64()).unwrap_or(self.double)
    }

    pub fn update_double(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.double = value.try_f64().unwrap_or(self.double.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "double", value);
        self.__load_state.mark_loaded("double").expect("generated field layout");
        self
    }

    pub fn changed_double(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "double")
    }

    pub fn eval_double(&self) -> teaql_core::eval::EvalResult<f64> {
        if !self.is_loaded("double") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "double".to_string(), attempted_path: "double".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.double())
                }}


    pub fn optional_double(&self) -> Option<f64> {
        self.optional_double.clone()
    }

    pub fn update_optional_double(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_double = if matches!(value, teaql_core::Value::Null) { None } else { value.try_f64().map(Some).unwrap_or_else(|| self.optional_double.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_double", value);
        self.__load_state.mark_loaded("optional_double").expect("generated field layout");
        self
    }

    pub fn changed_optional_double(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_double")
    }

    pub fn eval_optional_double(&self) -> teaql_core::eval::EvalResult<Option<f64>> {
        if !self.is_loaded("optional_double") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_double".to_string(), attempted_path: "optional_double".to_string() }
                } else {
                    match &self.optional_double {
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


    pub fn date(&self) -> chrono::NaiveDate {
        self.changed_date().and_then(|value| value.try_date()).unwrap_or(self.date)
    }

    pub fn update_date(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.date = value.try_date().unwrap_or(self.date.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "date", value);
        self.__load_state.mark_loaded("date").expect("generated field layout");
        self
    }

    pub fn changed_date(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "date")
    }

    pub fn eval_date(&self) -> teaql_core::eval::EvalResult<chrono::NaiveDate> {
        if !self.is_loaded("date") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "date".to_string(), attempted_path: "date".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.date())
                }}


    pub fn optional_date(&self) -> Option<chrono::NaiveDate> {
        self.optional_date.clone()
    }

    pub fn update_optional_date(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_date = if matches!(value, teaql_core::Value::Null) { None } else { value.try_date().map(Some).unwrap_or_else(|| self.optional_date.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_date", value);
        self.__load_state.mark_loaded("optional_date").expect("generated field layout");
        self
    }

    pub fn changed_optional_date(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_date")
    }

    pub fn eval_optional_date(&self) -> teaql_core::eval::EvalResult<Option<chrono::NaiveDate>> {
        if !self.is_loaded("optional_date") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_date".to_string(), attempted_path: "optional_date".to_string() }
                } else {
                    match &self.optional_date {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn time(&self) -> String {
        self.changed_time().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.time.clone())
    }

    pub fn update_time(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.time = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.time.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "time", value);
        self.__load_state.mark_loaded("time").expect("generated field layout");
        self
    }

    pub fn changed_time(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "time")
    }

    pub fn eval_time(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("time") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "time".to_string(), attempted_path: "time".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.time())
                }}


    pub fn optional_time(&self) -> Option<String> {
        self.optional_time.clone()
    }

    pub fn update_optional_time(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_time = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.optional_time.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_time", value);
        self.__load_state.mark_loaded("optional_time").expect("generated field layout");
        self
    }

    pub fn changed_optional_time(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_time")
    }

    pub fn eval_optional_time(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("optional_time") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_time".to_string(), attempted_path: "optional_time".to_string() }
                } else {
                    match &self.optional_time {
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


    pub fn optional_payload(&self) -> Option<serde_json::Value> {
        self.optional_payload.clone()
    }

    pub fn update_optional_payload(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_payload = if matches!(value, teaql_core::Value::Null) { None } else { Some(value.to_json_value()).map(Some).unwrap_or_else(|| self.optional_payload.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_payload", value);
        self.__load_state.mark_loaded("optional_payload").expect("generated field layout");
        self
    }

    pub fn changed_optional_payload(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_payload")
    }

    pub fn eval_optional_payload(&self) -> teaql_core::eval::EvalResult<Option<serde_json::Value>> {
        if !self.is_loaded("optional_payload") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_payload".to_string(), attempted_path: "optional_payload".to_string() }
                } else {
                    match &self.optional_payload {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn timestamp(&self) -> teaql_core::time::Timestamp {
        self.changed_timestamp().and_then(|value| value.try_timestamp()).unwrap_or(self.timestamp)
    }

    pub fn update_timestamp(&mut self, value: teaql_core::time::Timestamp) -> &mut Self {
        self.timestamp = value;
        let value = teaql_core::Value::from(value);
        self.__teaql_runtime_state().set(self.entity_key(), "timestamp", value);
        self.__load_state.mark_loaded("timestamp").expect("generated field layout");
        self
    }
    pub fn changed_timestamp(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "timestamp")
    }

    pub fn eval_timestamp(&self) -> teaql_core::eval::EvalResult<teaql_core::time::Timestamp> {
        if !self.is_loaded("timestamp") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "timestamp".to_string(), attempted_path: "timestamp".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.timestamp())
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

