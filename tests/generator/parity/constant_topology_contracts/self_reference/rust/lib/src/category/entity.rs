
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/category
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
#[teaql(reverse_relation(name = "category_list", target = "Category", json_type = "crate::Category", local_key = "id", foreign_key = "parent_id", many))]
#[teaql(reverse_relation(name = "status_list", target = "Status", json_type = "crate::Status", local_key = "id", foreign_key = "category_id", many))]
#[teaql(entity = "Category", table = "category_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Category {
// @source model.xml:2
#[teaql(id)]
    id: u64,

// @source model.xml:2
#[teaql(max_length = 100)]
    code: String,
#[teaql(version)]
    version: i64,
// @source model.xml:2
#[teaql(column = "platform")]
    platform_id: u64,

// @source model.xml:2
#[teaql(column = "parent")]
    parent_id: Option<u64>,
// @source model.xml:2
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,

// @source model.xml:2
#[teaql(relation(target = "Category", local_key = "parent_id", foreign_key = "id"))]
    parent: Option<Box<crate::Category>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Category {
    pub const ENTITY_NAME: &'static str = "category";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "f125609a17d099b17cd0cff4dcb25fb7d5edf3f09c2eca2d96327382693ed4f3";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("parent", 2),
        ("code", 3),
        ("version", 4),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("parent", "parent_id", "parent"),
        ("code", "code", "code"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            code: String::new(),
            version: 0_i64,
            platform_id: 0_u64,
            parent_id: None,
            platform: None,
            parent: None,
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
        if let Some(entity) = &mut self.parent {
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

    pub fn parent_id(&self) -> Option<u64> {
        self.parent_id.clone()
    }

    pub(crate) fn update_parent_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.parent_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.parent_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "parent_id", value);
        self.__load_state.mark_loaded("parent_id").expect("generated field layout");
        self
    }

    pub fn changed_parent_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "parent_id")
    }

    pub fn eval_parent_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("parent_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "parent_id".to_string(), attempted_path: "parent_id".to_string() }
                } else {
                    match &self.parent_id {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}
    pub fn update_parent_to_c1(&mut self) -> &mut Self {
        self.update_parent_id(1_u64)
    }

    pub fn parent_is_c1(&self) -> bool {
        self.parent_id() == 1_u64
    }
    pub fn update_parent_to_c22(&mut self) -> &mut Self {
        self.update_parent_id(22_u64)
    }

    pub fn parent_is_c22(&self) -> bool {
        self.parent_id() == 22_u64
    }
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

    pub fn parent(&self) -> Option<&crate::Category> {
        self.parent.as_deref().or_else(|| {
            if !self.is_loaded("parent") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Category>(
                Self::ENTITY_NAME, self.id(), "parent",
            ) {
                return edge.as_ref();
            }
            self.parent_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_parent(&self) -> teaql_core::eval::EvalResult<&crate::Category> {
        match self.parent() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("parent_id") && self.parent_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "parent".to_string(), attempted_path: "parent".to_string() },
        }
    }
    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn category_list(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::Category>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "category_list",
        )
    }

    pub fn eval_category_list(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::Category>> {
        let relation = self.category_list();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "category_list".to_string(), attempted_path: "category_list".to_string() },
        }
    }

    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn status_list(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::Status>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "status_list",
        )
    }

    pub fn eval_status_list(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::Status>> {
        let relation = self.status_list();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "status_list".to_string(), attempted_path: "status_list".to_string() },
        }
    }

}

