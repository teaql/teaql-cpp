
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

// @source model.xml:5
#[teaql(max_length = 100)]
    name: String,
#[teaql(version)]
    version: i64,
// @source model.xml:5
#[teaql(column = "platform")]
    platform_id: u64,

// @source model.xml:5
#[teaql(column = "namespace")]
    namespace_id: u64,

// @source model.xml:5
#[teaql(column = "operator")]
    operator_id: Option<u64>,
// @source model.xml:5
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,

// @source model.xml:5
#[teaql(relation(target = "Owner", local_key = "namespace_id", foreign_key = "id"))]
    namespace: Option<Box<crate::Owner>>,

// @source model.xml:5
#[teaql(relation(target = "Owner", local_key = "operator_id", foreign_key = "id"))]
    operator: Option<Box<crate::Owner>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Entry {
    pub const ENTITY_NAME: &'static str = "entry";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "afb92d842e26eb66afd50c072e28d299f7b4e282fd00a26595421857c898b4cf";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("name", 2),
        ("namespace", 3),
        ("operator", 4),
        ("version", 5),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("name", "name", "name"),
        ("namespace", "namespace_id", "namespace"),
        ("operator", "operator_id", "operator"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            name: String::new(),
            version: 0_i64,
            platform_id: 0_u64,
            namespace_id: 0_u64,
            operator_id: None,
            platform: None,
            namespace: None,
            operator: None,
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
        if let Some(entity) = &mut self.namespace {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.operator {
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

    pub fn namespace_id(&self) -> u64 {
        self.changed_namespace_id().and_then(|value| value.try_u64()).unwrap_or(self.namespace_id)
    }

    pub fn update_namespace_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.namespace_id = value.try_u64().unwrap_or(self.namespace_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "namespace_id", value);
        self.__load_state.mark_loaded("namespace_id").expect("generated field layout");
        self
    }

    pub fn changed_namespace_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "namespace_id")
    }

    pub fn eval_namespace_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("namespace_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "namespace_id".to_string(), attempted_path: "namespace_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.namespace_id())
                }}

    pub fn operator_id(&self) -> Option<u64> {
        self.operator_id.clone()
    }

    pub fn update_operator_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.operator_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.operator_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "operator_id", value);
        self.__load_state.mark_loaded("operator_id").expect("generated field layout");
        self
    }

    pub fn changed_operator_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "operator_id")
    }

    pub fn eval_operator_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("operator_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "operator_id".to_string(), attempted_path: "operator_id".to_string() }
                } else {
                    match &self.operator_id {
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

    pub fn namespace(&self) -> Option<&crate::Owner> {
        self.namespace.as_deref().or_else(|| {
            if !self.is_loaded("namespace") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Owner>(
                Self::ENTITY_NAME, self.id(), "namespace",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.namespace_id())})
    }

    pub fn eval_namespace(&self) -> teaql_core::eval::EvalResult<&crate::Owner> {
        match self.namespace() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "namespace".to_string(), attempted_path: "namespace".to_string() },
        }
    }

    pub fn operator(&self) -> Option<&crate::Owner> {
        self.operator.as_deref().or_else(|| {
            if !self.is_loaded("operator") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Owner>(
                Self::ENTITY_NAME, self.id(), "operator",
            ) {
                return edge.as_ref();
            }
            self.operator_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_operator(&self) -> teaql_core::eval::EvalResult<&crate::Owner> {
        match self.operator() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("operator_id") && self.operator_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "operator".to_string(), attempted_path: "operator".to_string() },
        }
    }

}

