
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

// @source checker_clock_composition.xml:8
#[teaql(max_length = 100)]
    code: String,

// @source checker_clock_composition.xml:8
    count: i64,

// @source checker_clock_composition.xml:8
#[teaql(large_text)]
    payload: serde_json::Value,
#[teaql(version)]
    version: i64,
// @source checker_clock_composition.xml:12
#[teaql(column = "owner")]
    owner_id: u64,

// @source checker_clock_composition.xml:12
#[teaql(column = "linked")]
    linked_id: u64,

// @source checker_clock_composition.xml:12
#[teaql(column = "optional_linked")]
    optional_linked_id: Option<u64>,
// @source checker_clock_composition.xml:12
#[teaql(relation(target = "Tenant", local_key = "owner_id", foreign_key = "id"))]
    owner: Option<Box<crate::Tenant>>,

// @source checker_clock_composition.xml:12
#[teaql(relation(target = "Tenant", local_key = "linked_id", foreign_key = "id"))]
    linked: Option<Box<crate::Tenant>>,

// @source checker_clock_composition.xml:12
#[teaql(relation(target = "Tenant", local_key = "optional_linked_id", foreign_key = "id"))]
    optional_linked: Option<Box<crate::Tenant>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Note {
    pub const ENTITY_NAME: &'static str = "note";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "5e3b709f11001c28f2369d9bd01217e07588f72d3fb9684ebabe881ad7fc739e";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("code", 1),
        ("count", 2),
        ("payload", 3),
        ("version", 4),
        ("owner", 5),
        ("linked", 6),
        ("optional_linked", 7),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("code", "code", "code"),
        ("count", "count", "count"),
        ("payload", "payload", "payload"),
        ("version", "version", "version"),
        ("owner", "owner_id", "owner"),
        ("linked", "linked_id", "linked"),
        ("optional_linked", "optional_linked_id", "optional_linked"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            code: String::new(),
            count: 0_i64,
            payload: serde_json::Value::Null,
            version: 0_i64,
            owner_id: 0_u64,
            linked_id: 0_u64,
            optional_linked_id: None,
            owner: None,
            linked: None,
            optional_linked: None,
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
        if let Some(entity) = &mut self.owner {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.linked {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.optional_linked {
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


    pub fn count(&self) -> i64 {
        self.changed_count().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.count)
    }

    pub fn update_count(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.count = value.try_i64().map(|value| value as i64).unwrap_or(self.count.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "count", value);
        self.__load_state.mark_loaded("count").expect("generated field layout");
        self
    }

    pub fn changed_count(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "count")
    }

    pub fn eval_count(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("count") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "count".to_string(), attempted_path: "count".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.count())
                }}


    pub fn payload(&self) -> serde_json::Value {
        self.payload.clone()
    }

    pub fn update_payload(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.payload = value.to_json_value();
        self.__teaql_runtime_state().set(self.entity_key(), "payload", value);
        self.__load_state.mark_loaded("payload").expect("generated field layout");
        self
    }

    pub fn changed_payload(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "payload")
    }

    pub fn eval_payload(&self) -> teaql_core::eval::EvalResult<serde_json::Value> {
        if !self.is_loaded("payload") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "payload".to_string(), attempted_path: "payload".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.payload())
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

    pub fn owner_id(&self) -> u64 {
        self.changed_owner_id().and_then(|value| value.try_u64()).unwrap_or(self.owner_id)
    }

    pub(crate) fn update_owner_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.owner_id = value.try_u64().unwrap_or(self.owner_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "owner_id", value);
        self.__load_state.mark_loaded("owner_id").expect("generated field layout");
        self
    }

    pub fn changed_owner_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "owner_id")
    }

    pub fn eval_owner_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("owner_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "owner_id".to_string(), attempted_path: "owner_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.owner_id())
                }}

    pub fn linked_id(&self) -> u64 {
        self.changed_linked_id().and_then(|value| value.try_u64()).unwrap_or(self.linked_id)
    }

    pub fn update_linked_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.linked_id = value.try_u64().unwrap_or(self.linked_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "linked_id", value);
        self.__load_state.mark_loaded("linked_id").expect("generated field layout");
        self
    }

    pub fn changed_linked_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "linked_id")
    }

    pub fn eval_linked_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("linked_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "linked_id".to_string(), attempted_path: "linked_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.linked_id())
                }}

    pub fn optional_linked_id(&self) -> Option<u64> {
        self.optional_linked_id.clone()
    }

    pub fn update_optional_linked_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.optional_linked_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.optional_linked_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "optional_linked_id", value);
        self.__load_state.mark_loaded("optional_linked_id").expect("generated field layout");
        self
    }

    pub fn changed_optional_linked_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "optional_linked_id")
    }

    pub fn eval_optional_linked_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("optional_linked_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_linked_id".to_string(), attempted_path: "optional_linked_id".to_string() }
                } else {
                    match &self.optional_linked_id {
                        Some(v) => teaql_core::eval::EvalResult::Value(Some(v.clone())),
                        None => teaql_core::eval::EvalResult::Null,
                    }
                }}
    pub fn owner(&self) -> Option<&crate::Tenant> {
        self.owner.as_deref().or_else(|| {
            if !self.is_loaded("owner") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Tenant>(
                Self::ENTITY_NAME, self.id(), "owner",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.owner_id())})
    }

    pub fn eval_owner(&self) -> teaql_core::eval::EvalResult<&crate::Tenant> {
        match self.owner() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "owner".to_string(), attempted_path: "owner".to_string() },
        }
    }

    pub fn linked(&self) -> Option<&crate::Tenant> {
        self.linked.as_deref().or_else(|| {
            if !self.is_loaded("linked") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Tenant>(
                Self::ENTITY_NAME, self.id(), "linked",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.linked_id())})
    }

    pub fn eval_linked(&self) -> teaql_core::eval::EvalResult<&crate::Tenant> {
        match self.linked() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "linked".to_string(), attempted_path: "linked".to_string() },
        }
    }

    pub fn optional_linked(&self) -> Option<&crate::Tenant> {
        self.optional_linked.as_deref().or_else(|| {
            if !self.is_loaded("optional_linked") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Tenant>(
                Self::ENTITY_NAME, self.id(), "optional_linked",
            ) {
                return edge.as_ref();
            }
            self.optional_linked_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_optional_linked(&self) -> teaql_core::eval::EvalResult<&crate::Tenant> {
        match self.optional_linked() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("optional_linked_id") && self.optional_linked_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "optional_linked".to_string(), attempted_path: "optional_linked".to_string() },
        }
    }

}

