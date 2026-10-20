
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/platform
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
#[teaql(reverse_relation(name = "priority_list", target = "Priority", json_type = "crate::Priority", local_key = "id", foreign_key = "platform_id", many))]
#[teaql(reverse_relation(name = "label_list", target = "Label", json_type = "crate::Label", local_key = "id", foreign_key = "platform_id", many))]
#[teaql(entity = "Platform", table = "platform_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Platform {
#[teaql(id)]
    id: u64,

// @source constants_temporal.xml:12
#[teaql(max_length = 100)]
    name: String,

// @source constants_temporal.xml:12
    created_at: teaql_core::time::Timestamp,

// @source constants_temporal.xml:12
    updated_at: teaql_core::time::Timestamp,
#[teaql(version)]
    version: i64,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Platform {
    pub const ENTITY_NAME: &'static str = "platform";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "a6c0e7a5d0f127d34e0082bc66b44bf36828961b848cc9215a64ffa384153210";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("name", 1),
        ("created_at", 2),
        ("updated_at", 3),
        ("version", 4),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("name", "name", "name"),
        ("created_at", "created_at", "created_at"),
        ("updated_at", "updated_at", "updated_at"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            name: String::new(),
            created_at: teaql_core::time::Timestamp::now(),
            updated_at: teaql_core::time::Timestamp::now(),
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


    pub fn created_at(&self) -> teaql_core::time::Timestamp {
        self.changed_created_at().and_then(|value| value.try_timestamp()).unwrap_or(self.created_at)
    }

    pub fn update_created_at(&mut self, value: teaql_core::time::Timestamp) -> &mut Self {
        self.created_at = value;
        let value = teaql_core::Value::from(value);
        self.__teaql_runtime_state().set(self.entity_key(), "created_at", value);
        self.__load_state.mark_loaded("created_at").expect("generated field layout");
        self
    }
    pub fn changed_created_at(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "created_at")
    }

    pub fn eval_created_at(&self) -> teaql_core::eval::EvalResult<teaql_core::time::Timestamp> {
        if !self.is_loaded("created_at") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "created_at".to_string(), attempted_path: "created_at".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.created_at())
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
    pub fn priority_list(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::Priority>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "priority_list",
        )
    }

    pub fn eval_priority_list(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::Priority>> {
        let relation = self.priority_list();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "priority_list".to_string(), attempted_path: "priority_list".to_string() },
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

