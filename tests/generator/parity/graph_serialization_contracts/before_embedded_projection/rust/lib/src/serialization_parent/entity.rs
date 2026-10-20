
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/serialization_parent
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
#[teaql(reverse_relation(name = "child_rows", target = "SerializationMember", json_type = "crate::SerializationMember", local_key = "id", foreign_key = "parent_id", many))]
#[teaql(reverse_relation(name = "boxed_children", target = "SerializationMember", json_type = "crate::SerializationMember", local_key = "id", foreign_key = "boxed_parent_id", many))]
#[teaql(entity = "SerializationParent", table = "serialization_parent_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct SerializationParent {
#[teaql(id)]
    id: u64,

// @source model.xml:8
#[teaql(max_length = 100)]
    name: String,
#[teaql(version)]
    version: i64,
// @source model.xml:8
#[teaql(column = "platform")]
    platform_id: u64,

// @source model.xml:8
#[teaql(column = "child")]
    child_id: Option<u64>,

// @source model.xml:8
#[teaql(column = "boxed_child")]
    boxed_child_id: Option<u64>,
// @source model.xml:8
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,

// @source model.xml:8
#[teaql(relation(target = "JsonGraphRow", local_key = "child_id", foreign_key = "id"))]
    child: Option<Box<crate::JsonGraphRow>>,

// @source model.xml:8
#[teaql(relation(target = "JsonGraphRow", local_key = "boxed_child_id", foreign_key = "id"))]
    boxed_child: Option<Box<crate::JsonGraphRow>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl SerializationParent {
    pub const ENTITY_NAME: &'static str = "serialization_parent";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "ea1abcde989beb5f26ba59911ea296719ea6a2940991bce6334e4816f6a44c13";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("name", 2),
        ("child", 3),
        ("boxed_child", 4),
        ("version", 5),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("name", "name", "name"),
        ("child", "child_id", "child"),
        ("boxed_child", "boxed_child_id", "boxed_child"),
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
            child_id: None,
            boxed_child_id: None,
            platform: None,
            child: None,
            boxed_child: None,
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
        if let Some(entity) = &mut self.child {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.boxed_child {
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

    pub fn child_id(&self) -> Option<u64> {
        self.child_id.clone()
    }

    pub fn update_child_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.child_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.child_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "child_id", value);
        self.__load_state.mark_loaded("child_id").expect("generated field layout");
        self
    }

    pub fn changed_child_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "child_id")
    }

    pub fn eval_child_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("child_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "child_id".to_string(), attempted_path: "child_id".to_string() }
                } else {
                    match &self.child_id {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}

    pub fn boxed_child_id(&self) -> Option<u64> {
        self.boxed_child_id.clone()
    }

    pub fn update_boxed_child_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.boxed_child_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.boxed_child_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "boxed_child_id", value);
        self.__load_state.mark_loaded("boxed_child_id").expect("generated field layout");
        self
    }

    pub fn changed_boxed_child_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "boxed_child_id")
    }

    pub fn eval_boxed_child_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("boxed_child_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "boxed_child_id".to_string(), attempted_path: "boxed_child_id".to_string() }
                } else {
                    match &self.boxed_child_id {
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

    pub fn child(&self) -> Option<&crate::JsonGraphRow> {
        self.child.as_deref().or_else(|| {
            if !self.is_loaded("child") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::JsonGraphRow>(
                Self::ENTITY_NAME, self.id(), "child",
            ) {
                return edge.as_ref();
            }
            self.child_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_child(&self) -> teaql_core::eval::EvalResult<&crate::JsonGraphRow> {
        match self.child() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("child_id") && self.child_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "child".to_string(), attempted_path: "child".to_string() },
        }
    }

    pub fn boxed_child(&self) -> Option<&crate::JsonGraphRow> {
        self.boxed_child.as_deref().or_else(|| {
            if !self.is_loaded("boxed_child") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::JsonGraphRow>(
                Self::ENTITY_NAME, self.id(), "boxed_child",
            ) {
                return edge.as_ref();
            }
            self.boxed_child_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_boxed_child(&self) -> teaql_core::eval::EvalResult<&crate::JsonGraphRow> {
        match self.boxed_child() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("boxed_child_id") && self.boxed_child_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "boxed_child".to_string(), attempted_path: "boxed_child".to_string() },
        }
    }
    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn child_rows(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::SerializationMember>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "child_rows",
        )
    }

    pub fn eval_child_rows(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::SerializationMember>> {
        let relation = self.child_rows();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "child_rows".to_string(), attempted_path: "child_rows".to_string() },
        }
    }

    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn boxed_children(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::SerializationMember>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "boxed_children",
        )
    }

    pub fn eval_boxed_children(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::SerializationMember>> {
        let relation = self.boxed_children();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "boxed_children".to_string(), attempted_path: "boxed_children".to_string() },
        }
    }

}

