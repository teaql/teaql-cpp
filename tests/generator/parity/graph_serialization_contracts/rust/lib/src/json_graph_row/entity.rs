
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/json_graph_row
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
#[teaql(reverse_relation(name = "single_child", target = "JsonGraphRow", json_type = "crate::JsonGraphRow", local_key = "id", foreign_key = "single_peer_id"))]
#[teaql(reverse_relation(name = "child_rows", target = "JsonGraphRow", json_type = "crate::JsonGraphRow", local_key = "id", foreign_key = "peer_id", many))]
#[teaql(reverse_relation(name = "parents", target = "SerializationParent", json_type = "crate::SerializationParent", local_key = "id", foreign_key = "child_id", many))]
#[teaql(reverse_relation(name = "boxed_parents", target = "SerializationParent", json_type = "crate::SerializationParent", local_key = "id", foreign_key = "boxed_child_id", many))]
#[teaql(entity = "JsonGraphRow", table = "json_graph_row_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct JsonGraphRow {
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
#[teaql(column = "peer")]
    peer_id: Option<u64>,

// @source model.xml:5
#[teaql(column = "single_peer")]
    single_peer_id: Option<u64>,
// @source model.xml:5
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,

// @source model.xml:5
#[teaql(relation(target = "JsonGraphRow", local_key = "peer_id", foreign_key = "id"))]
    peer: Option<Box<crate::JsonGraphRow>>,

// @source model.xml:5
#[teaql(relation(target = "JsonGraphRow", local_key = "single_peer_id", foreign_key = "id"))]
    single_peer: Option<Box<crate::JsonGraphRow>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl JsonGraphRow {
    pub const ENTITY_NAME: &'static str = "json_graph_row";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "105deed0c9748a63590cdc126fe2a73ccffdfa968e4fc89c1d1ec0236d27fa7e";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("name", 2),
        ("peer", 3),
        ("single_peer", 4),
        ("version", 5),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("name", "name", "name"),
        ("peer", "peer_id", "peer"),
        ("single_peer", "single_peer_id", "single_peer"),
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
            peer_id: None,
            single_peer_id: None,
            platform: None,
            peer: None,
            single_peer: None,
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
        if let Some(entity) = &mut self.peer {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.single_peer {
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

    pub fn peer_id(&self) -> Option<u64> {
        self.peer_id.clone()
    }

    pub fn update_peer_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.peer_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.peer_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "peer_id", value);
        self.__load_state.mark_loaded("peer_id").expect("generated field layout");
        self
    }

    pub fn changed_peer_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "peer_id")
    }

    pub fn eval_peer_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("peer_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "peer_id".to_string(), attempted_path: "peer_id".to_string() }
                } else {
                    match &self.peer_id {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}

    pub fn single_peer_id(&self) -> Option<u64> {
        self.single_peer_id.clone()
    }

    pub fn update_single_peer_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.single_peer_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.single_peer_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "single_peer_id", value);
        self.__load_state.mark_loaded("single_peer_id").expect("generated field layout");
        self
    }

    pub fn changed_single_peer_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "single_peer_id")
    }

    pub fn eval_single_peer_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("single_peer_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "single_peer_id".to_string(), attempted_path: "single_peer_id".to_string() }
                } else {
                    match &self.single_peer_id {
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

    pub fn peer(&self) -> Option<&crate::JsonGraphRow> {
        self.peer.as_deref().or_else(|| {
            if !self.is_loaded("peer") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::JsonGraphRow>(
                Self::ENTITY_NAME, self.id(), "peer",
            ) {
                return edge.as_ref();
            }
            self.peer_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_peer(&self) -> teaql_core::eval::EvalResult<&crate::JsonGraphRow> {
        match self.peer() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("peer_id") && self.peer_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "peer".to_string(), attempted_path: "peer".to_string() },
        }
    }

    pub fn single_peer(&self) -> Option<&crate::JsonGraphRow> {
        self.single_peer.as_deref().or_else(|| {
            if !self.is_loaded("single_peer") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::JsonGraphRow>(
                Self::ENTITY_NAME, self.id(), "single_peer",
            ) {
                return edge.as_ref();
            }
            self.single_peer_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_single_peer(&self) -> teaql_core::eval::EvalResult<&crate::JsonGraphRow> {
        match self.single_peer() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("single_peer_id") && self.single_peer_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "single_peer".to_string(), attempted_path: "single_peer".to_string() },
        }
    }
    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn single_child(&self) -> teaql_runtime::RelationHandle<'_, crate::JsonGraphRow> {
        self.__teaql_runtime_state().relation_option(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "single_child",
        )
    }

    pub fn eval_single_child(&self) -> teaql_core::eval::EvalResult<&crate::JsonGraphRow> {
        let relation = self.single_child();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded relation must have a value")),
            teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Null,
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "single_child".to_string(), attempted_path: "single_child".to_string() },
        }
    }
    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn child_rows(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::JsonGraphRow>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "child_rows",
        )
    }

    pub fn eval_child_rows(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::JsonGraphRow>> {
        let relation = self.child_rows();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "child_rows".to_string(), attempted_path: "child_rows".to_string() },
        }
    }

    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn parents(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::SerializationParent>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "parents",
        )
    }

    pub fn eval_parents(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::SerializationParent>> {
        let relation = self.parents();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "parents".to_string(), attempted_path: "parents".to_string() },
        }
    }

    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn boxed_parents(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::SerializationParent>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "boxed_parents",
        )
    }

    pub fn eval_boxed_parents(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::SerializationParent>> {
        let relation = self.boxed_parents();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "boxed_parents".to_string(), attempted_path: "boxed_parents".to_string() },
        }
    }

}

