
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/person
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
#[teaql(reverse_relation(name = "device_list", target = "Device", json_type = "crate::Device", local_key = "id", foreign_key = "owner_id", many))]
#[teaql(entity = "Person", table = "person_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Person {
#[teaql(id)]
    id: u64,

// @source types.xml:4
#[teaql(max_length = 100)]
    email: String,

// @source types.xml:4
    active: bool,

// @source types.xml:4
#[teaql(max_length = 100)]
    nickname: Option<String>,

// @source types.xml:4
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    balance: rust_decimal::Decimal,

// @source types.xml:4
    height: f64,

// @source types.xml:4
    ratio: f32,

// @source types.xml:4
    age: i64,

// @source types.xml:4
    birthday: chrono::NaiveDate,

// @source types.xml:4
    updated_at: teaql_core::time::Timestamp,

// @source types.xml:4
#[teaql(large_text)]
    payload: Option<serde_json::Value>,
#[teaql(version)]
    version: i64,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Person {
    pub const ENTITY_NAME: &'static str = "person";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "90157748987c0f22ce5ad59c19e3ad2830f684cf596c6b0b7a7d7325ef5e23ed";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("email", 1),
        ("active", 2),
        ("nickname", 3),
        ("balance", 4),
        ("height", 5),
        ("ratio", 6),
        ("age", 7),
        ("birthday", 8),
        ("updated_at", 9),
        ("payload", 10),
        ("version", 11),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("email", "email", "email"),
        ("active", "active", "active"),
        ("nickname", "nickname", "nickname"),
        ("balance", "balance", "balance"),
        ("height", "height", "height"),
        ("ratio", "ratio", "ratio"),
        ("age", "age", "age"),
        ("birthday", "birthday", "birthday"),
        ("updated_at", "updated_at", "updated_at"),
        ("payload", "payload", "payload"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            email: String::new(),
            active: false,
            nickname: None,
            balance: rust_decimal::Decimal::ZERO,
            height: 0.0_f64,
            ratio: 0.0_f32,
            age: 0_i64,
            birthday: chrono::NaiveDate::from_ymd_opt(1970, 1, 1).unwrap(),
            updated_at: teaql_core::time::Timestamp::now(),
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


    pub fn email(&self) -> String {
        self.changed_email().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.email.clone())
    }

    pub fn update_email(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.email = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.email.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "email", value);
        self.__load_state.mark_loaded("email").expect("generated field layout");
        self
    }

    pub fn changed_email(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "email")
    }

    pub fn eval_email(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("email") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "email".to_string(), attempted_path: "email".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.email())
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


    pub fn nickname(&self) -> Option<String> {
        self.nickname.clone()
    }

    pub fn update_nickname(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.nickname = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.nickname.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "nickname", value);
        self.__load_state.mark_loaded("nickname").expect("generated field layout");
        self
    }

    pub fn changed_nickname(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "nickname")
    }

    pub fn eval_nickname(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("nickname") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "nickname".to_string(), attempted_path: "nickname".to_string() }
                } else {
                    match &self.nickname {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn balance(&self) -> rust_decimal::Decimal {
        self.changed_balance().and_then(|value| value.try_decimal()).unwrap_or(self.balance)
    }

    pub fn update_balance(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.balance = value.try_decimal().unwrap_or(self.balance.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "balance", value);
        self.__load_state.mark_loaded("balance").expect("generated field layout");
        self
    }

    pub fn changed_balance(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "balance")
    }

    pub fn eval_balance(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("balance") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "balance".to_string(), attempted_path: "balance".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.balance())
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


    pub fn age(&self) -> i64 {
        self.changed_age().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.age)
    }

    pub fn update_age(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.age = value.try_i64().map(|value| value as i64).unwrap_or(self.age.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "age", value);
        self.__load_state.mark_loaded("age").expect("generated field layout");
        self
    }

    pub fn changed_age(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "age")
    }

    pub fn eval_age(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("age") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "age".to_string(), attempted_path: "age".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.age())
                }}


    pub fn birthday(&self) -> chrono::NaiveDate {
        self.changed_birthday().and_then(|value| value.try_date()).unwrap_or(self.birthday)
    }

    pub fn update_birthday(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.birthday = value.try_date().unwrap_or(self.birthday.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "birthday", value);
        self.__load_state.mark_loaded("birthday").expect("generated field layout");
        self
    }

    pub fn changed_birthday(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "birthday")
    }

    pub fn eval_birthday(&self) -> teaql_core::eval::EvalResult<chrono::NaiveDate> {
        if !self.is_loaded("birthday") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "birthday".to_string(), attempted_path: "birthday".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.birthday())
                }}


    pub fn updated_at(&self) -> teaql_core::time::Timestamp {
        self.changed_updated_at().and_then(|value| value.try_timestamp()).unwrap_or(self.updated_at)
    }

    pub fn update_updated_at(&mut self, value: teaql_core::time::Timestamp) -> &mut Self {
        self.updated_at = value;
        let value = teaql_core::Value::from(value);
        self.__teaql_runtime_state().set(self.entity_key(), "updated_at", value);
        self.__load_state.mark_loaded("updated_at").expect("generated field layout");
        self
    }
    pub fn changed_updated_at(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "updated_at")
    }

    pub fn eval_updated_at(&self) -> teaql_core::eval::EvalResult<teaql_core::time::Timestamp> {
        if !self.is_loaded("updated_at") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "updated_at".to_string(), attempted_path: "updated_at".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.updated_at())
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

    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn device_list(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::Device>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "device_list",
        )
    }

    pub fn eval_device_list(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::Device>> {
        let relation = self.device_list();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "device_list".to_string(), attempted_path: "device_list".to_string() },
        }
    }

}

