
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/research_academy
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
#[teaql(entity = "ResearchAcademy", table = "research_academy_data", data_service = "sqlite", audit_mask_fields = "name")]
pub struct ResearchAcademy {
#[teaql(id)]
    id: u64,

// @source inheritance_graph.xml:6
#[teaql(max_length = 100)]
    name: String,

// @source inheritance_graph.xml:6
    rank: i64,
#[teaql(version)]
    version: i64,

// @source inheritance_graph.xml:7
#[teaql(max_length = 100)]
    campus_code: String,

// @source inheritance_graph.xml:8
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    budget: rust_decimal::Decimal,
// @source inheritance_graph.xml:6
#[teaql(column = "platform")]
    platform_id: u64,

// @source inheritance_graph.xml:6
#[teaql(column = "advisor")]
    advisor_id: Option<u64>,
// @source inheritance_graph.xml:6
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,

// @source inheritance_graph.xml:6
#[teaql(relation(target = "Specialist", local_key = "advisor_id", foreign_key = "id"))]
    advisor: Option<Box<crate::Specialist>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl ResearchAcademy {
    pub const ENTITY_NAME: &'static str = "research_academy";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "d119e5465ab69f7e6ebe15364f6fbdb2bb9824640b7957d47e7e05f2e371777a";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("name", 2),
        ("rank", 3),
        ("advisor", 4),
        ("version", 5),
        ("campus_code", 6),
        ("budget", 7),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("name", "name", "name"),
        ("rank", "rank", "rank"),
        ("advisor", "advisor_id", "advisor"),
        ("version", "version", "version"),
        ("campus_code", "campus_code", "campus_code"),
        ("budget", "budget", "budget"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            name: String::new(),
            rank: 0_i64,
            version: 0_i64,
            campus_code: String::new(),
            budget: rust_decimal::Decimal::ZERO,
            platform_id: 0_u64,
            advisor_id: None,
            platform: None,
            advisor: None,
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
        if let Some(entity) = &mut self.advisor {
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


    pub fn campus_code(&self) -> String {
        self.changed_campus_code().and_then(|value| value.try_text().map(|value| value.to_owned())).unwrap_or_else(|| self.campus_code.clone())
    }

    pub fn update_campus_code(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.campus_code = value.try_text().map(|value| value.trim().to_owned()).unwrap_or_else(|| self.campus_code.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "campus_code", value);
        self.__load_state.mark_loaded("campus_code").expect("generated field layout");
        self
    }

    pub fn changed_campus_code(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "campus_code")
    }

    pub fn eval_campus_code(&self) -> teaql_core::eval::EvalResult<String> {
        if !self.is_loaded("campus_code") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "campus_code".to_string(), attempted_path: "campus_code".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.campus_code())
                }}


    pub fn budget(&self) -> rust_decimal::Decimal {
        self.changed_budget().and_then(|value| value.try_decimal()).unwrap_or(self.budget)
    }

    pub fn update_budget(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.budget = value.try_decimal().unwrap_or(self.budget.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "budget", value);
        self.__load_state.mark_loaded("budget").expect("generated field layout");
        self
    }

    pub fn changed_budget(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "budget")
    }

    pub fn eval_budget(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("budget") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "budget".to_string(), attempted_path: "budget".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.budget())
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

    pub fn advisor_id(&self) -> Option<u64> {
        self.advisor_id.clone()
    }

    pub fn update_advisor_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.advisor_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.advisor_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "advisor_id", value);
        self.__load_state.mark_loaded("advisor_id").expect("generated field layout");
        self
    }

    pub fn changed_advisor_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "advisor_id")
    }

    pub fn eval_advisor_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("advisor_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "advisor_id".to_string(), attempted_path: "advisor_id".to_string() }
                } else {
                    match &self.advisor_id {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
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

    pub fn advisor(&self) -> Option<&crate::Specialist> {
        self.advisor.as_deref().or_else(|| {
            if !self.is_loaded("advisor") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Specialist>(
                Self::ENTITY_NAME, self.id(), "advisor",
            ) {
                return edge.as_ref();
            }
            self.advisor_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_advisor(&self) -> teaql_core::eval::EvalResult<&crate::Specialist> {
        match self.advisor() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("advisor_id") && self.advisor_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "advisor".to_string(), attempted_path: "advisor".to_string() },
        }
    }

}

