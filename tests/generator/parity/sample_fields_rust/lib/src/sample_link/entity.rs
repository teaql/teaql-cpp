
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/sample_link
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
#[teaql(entity = "SampleLink", table = "sample_link_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct SampleLink {
#[teaql(id)]
    id: u64,

// @source sample_fields.xml:13
#[teaql(max_length = 100)]
    name: String,
#[teaql(version)]
    version: i64,
// @source sample_fields.xml:13
#[teaql(column = "first_record")]
    first_record_id: u64,

// @source sample_fields.xml:13
#[teaql(column = "second_record")]
    second_record_id: u64,
// @source sample_fields.xml:13
#[teaql(relation(target = "SampleRecord", local_key = "first_record_id", foreign_key = "id"))]
    first_record: Option<Box<crate::SampleRecord>>,

// @source sample_fields.xml:13
#[teaql(relation(target = "SampleRecord", local_key = "second_record_id", foreign_key = "id"))]
    second_record: Option<Box<crate::SampleRecord>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl SampleLink {
    pub const ENTITY_NAME: &'static str = "sample_link";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "d74f18ed3688b14acb5450c4a6d5330893304e5057bf7efb34f90f4c30f4ec20";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("first_record", 1),
        ("second_record", 2),
        ("name", 3),
        ("version", 4),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("first_record", "first_record_id", "first_record"),
        ("second_record", "second_record_id", "second_record"),
        ("name", "name", "name"),
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
            first_record_id: 0_u64,
            second_record_id: 0_u64,
            first_record: None,
            second_record: None,
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
        if let Some(entity) = &mut self.first_record {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.second_record {
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

    pub fn first_record_id(&self) -> u64 {
        self.changed_first_record_id().and_then(|value| value.try_u64()).unwrap_or(self.first_record_id)
    }

    pub fn update_first_record_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.first_record_id = value.try_u64().unwrap_or(self.first_record_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "first_record_id", value);
        self.__load_state.mark_loaded("first_record_id").expect("generated field layout");
        self
    }

    pub fn changed_first_record_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "first_record_id")
    }

    pub fn eval_first_record_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("first_record_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "first_record_id".to_string(), attempted_path: "first_record_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.first_record_id())
                }}

    pub fn second_record_id(&self) -> u64 {
        self.changed_second_record_id().and_then(|value| value.try_u64()).unwrap_or(self.second_record_id)
    }

    pub fn update_second_record_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.second_record_id = value.try_u64().unwrap_or(self.second_record_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "second_record_id", value);
        self.__load_state.mark_loaded("second_record_id").expect("generated field layout");
        self
    }

    pub fn changed_second_record_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "second_record_id")
    }

    pub fn eval_second_record_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("second_record_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "second_record_id".to_string(), attempted_path: "second_record_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.second_record_id())
                }}
    pub fn first_record(&self) -> Option<&crate::SampleRecord> {
        self.first_record.as_deref().or_else(|| {
            if !self.is_loaded("first_record") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::SampleRecord>(
                Self::ENTITY_NAME, self.id(), "first_record",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.first_record_id())})
    }

    pub fn eval_first_record(&self) -> teaql_core::eval::EvalResult<&crate::SampleRecord> {
        match self.first_record() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "first_record".to_string(), attempted_path: "first_record".to_string() },
        }
    }

    pub fn second_record(&self) -> Option<&crate::SampleRecord> {
        self.second_record.as_deref().or_else(|| {
            if !self.is_loaded("second_record") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::SampleRecord>(
                Self::ENTITY_NAME, self.id(), "second_record",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.second_record_id())})
    }

    pub fn eval_second_record(&self) -> teaql_core::eval::EvalResult<&crate::SampleRecord> {
        match self.second_record() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "second_record".to_string(), attempted_path: "second_record".to_string() },
        }
    }

}

