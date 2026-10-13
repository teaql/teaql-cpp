
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/profile
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
#[teaql(entity = "Profile", table = "profile_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Profile {
#[teaql(id)]
    id: u64,

// @source dynamic_json.xml:4
#[teaql(max_length = 100)]
    note: Option<String>,
#[teaql(version)]
    version: i64,
// @source dynamic_json.xml:4
#[teaql(column = "person")]
    person_id: u64,
// @source dynamic_json.xml:4
#[teaql(relation(target = "Person", local_key = "person_id", foreign_key = "id"))]
    person: Option<Box<crate::Person>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Profile {
    pub const ENTITY_NAME: &'static str = "profile";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "666cec90a60a65b0c0fe07f7c6e1f28be2b8411f3b2307c84c6714dd255b3169";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("person", 1),
        ("note", 2),
        ("version", 3),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("person", "person_id", "person"),
        ("note", "note", "note"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            note: None,
            version: 0_i64,
            person_id: 0_u64,
            person: None,
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
        if let Some(entity) = &mut self.person {
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

    pub fn person_id(&self) -> u64 {
        self.changed_person_id().and_then(|value| value.try_u64()).unwrap_or(self.person_id)
    }

    pub fn update_person_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.person_id = value.try_u64().unwrap_or(self.person_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "person_id", value);
        self.__load_state.mark_loaded("person_id").expect("generated field layout");
        self
    }

    pub fn changed_person_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "person_id")
    }

    pub fn eval_person_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("person_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "person_id".to_string(), attempted_path: "person_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.person_id())
                }}
    pub fn person(&self) -> Option<&crate::Person> {
        self.person.as_deref().or_else(|| {
            if !self.is_loaded("person") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Person>(
                Self::ENTITY_NAME, self.id(), "person",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.person_id())})
    }

    pub fn eval_person(&self) -> teaql_core::eval::EvalResult<&crate::Person> {
        match self.person() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "person".to_string(), attempted_path: "person".to_string() },
        }
    }

}

