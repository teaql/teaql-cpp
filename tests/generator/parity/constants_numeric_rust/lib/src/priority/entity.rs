
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/priority
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
#[teaql(reverse_relation(name = "label_list", target = "Label", json_type = "crate::Label", local_key = "id", foreign_key = "priority_id", many))]
#[teaql(entity = "Priority", table = "priority_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Priority {
// @source constants_numeric.xml:4
#[teaql(id)]
    id: u64,

// @source constants_numeric.xml:4
#[teaql(max_length = 100)]
    name: String,

// @source constants_numeric.xml:4
#[teaql(max_length = 100)]
    code: String,

// @source constants_numeric.xml:4
    rank: i64,

// @source constants_numeric.xml:4
    weight: f64,

// @source constants_numeric.xml:4
    ratio: f32,

// @source constants_numeric.xml:4
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    cost: rust_decimal::Decimal,

// @source constants_numeric.xml:4
    day: chrono::NaiveDate,

// @source constants_numeric.xml:4
#[teaql(max_length = 100)]
    note: Option<String>,

// @source constants_numeric.xml:4
    optional_rank: Option<i64>,
#[teaql(version)]
    version: i64,
// @source constants_numeric.xml:4
#[teaql(column = "platform")]
    platform_id: u64,
// @source constants_numeric.xml:4
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Priority {
    pub const ENTITY_NAME: &'static str = "priority";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "692c0975bde0c90c12f71e9f6668faf4ba6f7ea96af50d56429cd5a1e4c8f16f";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("name", 2),
        ("code", 3),
        ("rank", 4),
        ("weight", 5),
        ("ratio", 6),
        ("cost", 7),
        ("day", 8),
        ("note", 9),
        ("optional_rank", 10),
        ("version", 11),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("name", "name", "name"),
        ("code", "code", "code"),
        ("rank", "rank", "rank"),
        ("weight", "weight", "weight"),
        ("ratio", "ratio", "ratio"),
        ("cost", "cost", "cost"),
        ("day", "day", "day"),
        ("note", "note", "note"),
        ("optional_rank", "optional_rank", "optional_rank"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            name: String::new(),
            code: String::new(),
            rank: 0_i64,
            weight: 0.0_f64,
            ratio: 0.0_f32,
            cost: rust_decimal::Decimal::ZERO,
            day: chrono::NaiveDate::from_ymd_opt(1970, 1, 1).unwrap(),
            note: None,
            optional_rank: None,
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


    pub fn name(&self) -> String {
        self.changed_name().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.name.clone())
    }

    pub fn update_name(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.name = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.name.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "name", value);
        self.__load_state.mark_loaded("name").expect("generated field layout");
        self
    }

    pub fn changed_name(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "name")
    }

    pub fn eval_name(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("name") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "name".to_string(), attempted_path: "name".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.name())
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


    pub fn rank(&self) -> i64 {
        self.changed_rank().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.rank)
    }

    pub fn update_rank(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.rank = value.try_i64().map(|value| value as i64).unwrap_or(self.rank.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "rank", value);
        self.__load_state.mark_loaded("rank").expect("generated field layout");
        self
    }

    pub fn changed_rank(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "rank")
    }

    pub fn eval_rank(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("rank") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "rank".to_string(), attempted_path: "rank".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.rank())
                }}


    pub fn weight(&self) -> f64 {
        self.changed_weight().and_then(|value| value.try_f64()).unwrap_or(self.weight)
    }

    pub fn update_weight(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.weight = value.try_f64().unwrap_or(self.weight.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "weight", value);
        self.__load_state.mark_loaded("weight").expect("generated field layout");
        self
    }

    pub fn changed_weight(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "weight")
    }

    pub fn eval_weight(&self) -> teaql_core::eval::EvalResult<f64> {
        if !self.is_loaded("weight") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "weight".to_string(), attempted_path: "weight".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.weight())
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


    pub fn cost(&self) -> rust_decimal::Decimal {
        self.changed_cost().and_then(|value| value.try_decimal()).unwrap_or(self.cost)
    }

    pub fn update_cost(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.cost = value.try_decimal().unwrap_or(self.cost.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "cost", value);
        self.__load_state.mark_loaded("cost").expect("generated field layout");
        self
    }

    pub fn changed_cost(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "cost")
    }

    pub fn eval_cost(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("cost") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "cost".to_string(), attempted_path: "cost".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.cost())
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


    pub fn note(&self) -> Option<String> {
        self.note.clone()
    }

    pub fn update_note(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.note = if matches!(value, teaql_core::Value::Null) { None } else { value.try_text().map(|value| value.trim().to_owned()).map(Some).unwrap_or_else(|| self.note.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "note", value);
        self.__load_state.mark_loaded("note").expect("generated field layout");
        self
    }

    pub fn changed_note(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "note")
    }

    pub fn eval_note(&self) -> teaql_core::eval::EvalResult<Option<String>> {
        if !self.is_loaded("note") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "note".to_string(), attempted_path: "note".to_string() }
                } else {
                    match &self.note {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}


    pub fn optional_rank(&self) -> Option<i64> {
        self.optional_rank.clone()
    }

    pub fn update_optional_rank(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_rank = if matches!(value, teaql_core::Value::Null) { None } else { value.try_i64().map(|value| value as i64).map(Some).unwrap_or_else(|| self.optional_rank.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_rank", value);
        self.__load_state.mark_loaded("optional_rank").expect("generated field layout");
        self
    }

    pub fn changed_optional_rank(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_rank")
    }

    pub fn eval_optional_rank(&self) -> teaql_core::eval::EvalResult<Option<i64>> {
        if !self.is_loaded("optional_rank") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_rank".to_string(), attempted_path: "optional_rank".to_string() }
                } else {
                    match &self.optional_rank {
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

    pub(crate) fn update_platform_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
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
    pub fn label_list(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::Label>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "label_list",
        )
    }

    pub fn eval_label_list(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::Label>> {
        let relation = self.label_list();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "label_list".to_string(), attempted_path: "label_list".to_string() },
        }
    }

}

