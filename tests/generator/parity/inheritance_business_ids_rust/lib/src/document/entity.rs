
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/document
use std::collections::BTreeMap;

use teaql_macros::{teaql_entity, TeaqlEntity};

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct DocumentNumber(String);

impl DocumentNumber {
    pub fn new(value: impl Into<String>) -> Result<Self, &'static str> {
        let value = value.into().trim().to_owned();
        if value.is_empty() {
            return Err("document_number must not be blank");
        }
        Ok(Self(value))
    }

    pub(crate) fn from_canonical(value: impl Into<String>) -> Self {
        Self(value.into())
    }

    pub fn as_str(&self) -> &str { &self.0 }

    pub fn definition() -> teaql_core::business_id::BusinessIdDefinition {
        teaql_core::business_id::BusinessIdDefinition {
            field_name: "document_number".to_owned(),
            profile: "daily-permuted-v1".to_owned(),
            prefix: "DOC".to_owned(),
            namespace: "documents".to_owned(),
            split_by_date: true,
            preserve_digits: 6,
            separator: "-".to_owned(),
        }
    }
}

impl From<DocumentNumber> for teaql_core::Value {
    fn from(value: DocumentNumber) -> Self {
        teaql_core::Value::Text(value.0)
    }
}

impl From<&DocumentNumber> for teaql_core::Value {
    fn from(value: &DocumentNumber) -> Self {
        teaql_core::Value::Text(value.0.clone())
    }
}

/// [TEAQL AI WARNING]
/// TeaQL was explicitly designed to PREVENT AI hallucinations and random guessing.
/// DO NOT GUESS METHOD NAMES!
/// The methods listed below are the ONLY valid ways to interact with this entity.
/// If you encounter compilation errors (e.g., method not found), DO NOT guess another method name.
/// Read the method signatures in this file before proceeding.
#[teaql_entity]
#[derive(Clone, Debug, PartialEq, TeaqlEntity)]
#[teaql(indexed_layout)]
#[teaql(entity = "Document", table = "document_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Document {
#[teaql(id)]
    id: u64,

// @source inheritance_business_ids.xml:5
#[teaql(max_length = 100)]
    name: String,

// @source inheritance_business_ids.xml:5
    active: bool,

// @source inheritance_business_ids.xml:5
#[teaql(max_length = 100)]
    note: Option<String>,

// @source inheritance_business_ids.xml:5
#[teaql(max_length = 100)]
    document_number: String,

// @source inheritance_business_ids.xml:5
    created_at: teaql_core::time::Timestamp,

// @source inheritance_business_ids.xml:5
    updated_at: teaql_core::time::Timestamp,
#[teaql(version)]
    version: i64,
// @source inheritance_business_ids.xml:5
#[teaql(column = "tenant")]
    tenant_id: u64,
// @source inheritance_business_ids.xml:5
#[teaql(relation(target = "Tenant", local_key = "tenant_id", foreign_key = "id"))]
    tenant: Option<Box<crate::Tenant>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Document {
    pub const ENTITY_NAME: &'static str = "document";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "5d8f3b97774538f533206ccfe50f914a5dc2c1ea9313f2bc8635f3f97e3d695a";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("tenant", 1),
        ("name", 2),
        ("active", 3),
        ("note", 4),
        ("document_number", 5),
        ("created_at", 6),
        ("updated_at", 7),
        ("version", 8),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("tenant", "tenant_id", "tenant"),
        ("name", "name", "name"),
        ("active", "active", "active"),
        ("note", "note", "note"),
        ("document_number", "document_number", "document_number"),
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
            active: false,
            note: None,
            document_number: String::new(),
            created_at: teaql_core::time::Timestamp::now(),
            updated_at: teaql_core::time::Timestamp::now(),
            version: 0_i64,
            tenant_id: 0_u64,
            tenant: None,
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
        if let Some(entity) = &mut self.tenant {
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


    pub fn active(&self) -> bool {
        self.changed_active().and_then(|value| value.try_bool()).unwrap_or(self.active)
    }

    pub fn update_active(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.active = value.try_bool().unwrap_or(self.active.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "active", value);
        self.__load_state.mark_loaded("active").expect("generated field layout");
        self
    }

    pub fn changed_active(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "active")
    }

    pub fn eval_active(&self) -> teaql_core::eval::EvalResult<bool> {
        if !self.is_loaded("active") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "active".to_string(), attempted_path: "active".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.active())
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


    pub fn document_number(&self) -> DocumentNumber {
        let value = self.changed_document_number()
            .and_then(|value| value.try_text().map(str::to_owned))
            .unwrap_or_else(|| self.document_number.clone());
        DocumentNumber::from_canonical(value)
    }

    pub fn update_document_number(&mut self, value: DocumentNumber) -> &mut Self {
        let canonical = value.0.clone();
        self.document_number = canonical.clone();
        self.__teaql_runtime_state().set(
            self.entity_key(),
            "document_number",
            teaql_core::Value::Text(canonical),
        );
        self.__load_state.mark_loaded("document_number").expect("generated field layout");
        self
    }

    pub fn changed_document_number(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "document_number")
    }

    pub fn eval_document_number(&self) -> teaql_core::eval::EvalResult<DocumentNumber> {
        if !self.is_loaded("document_number") {
            teaql_core::eval::EvalResult::NotLoaded {
                failed_node: "document_number".to_string(),
                attempted_path: "document_number".to_string(),
            }
        } else {
            teaql_core::eval::EvalResult::Value(self.document_number())
        }
    }


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

    pub fn tenant_id(&self) -> u64 {
        self.changed_tenant_id().and_then(|value| value.try_u64()).unwrap_or(self.tenant_id)
    }

    pub(crate) fn update_tenant_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.tenant_id = value.try_u64().unwrap_or(self.tenant_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "tenant_id", value);
        self.__load_state.mark_loaded("tenant_id").expect("generated field layout");
        self
    }

    pub fn changed_tenant_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "tenant_id")
    }

    pub fn eval_tenant_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("tenant_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "tenant_id".to_string(), attempted_path: "tenant_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.tenant_id())
                }}
    pub fn tenant(&self) -> Option<&crate::Tenant> {
        self.tenant.as_deref().or_else(|| {
            if !self.is_loaded("tenant") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Tenant>(
                Self::ENTITY_NAME, self.id(), "tenant",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.tenant_id())})
    }

    pub fn eval_tenant(&self) -> teaql_core::eval::EvalResult<&crate::Tenant> {
        match self.tenant() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "tenant".to_string(), attempted_path: "tenant".to_string() },
        }
    }

}

pub(crate) struct DocumentDocumentNumberBusinessIdSlot<'a>(
    pub(crate) &'a mut Document,
    pub(crate) bool,
);

impl teaql_core::business_id::BusinessIdSlot for DocumentDocumentNumberBusinessIdSlot<'_> {
    fn current_business_id(&self) -> Option<&str> {
        (!self.0.document_number.is_empty()).then_some(self.0.document_number.as_str())
    }

    fn is_new_aggregate(&self) -> bool {
        self.1
    }

    fn assign_business_id(&mut self, value: teaql_core::business_id::BusinessIdValue) {
        self.0.update_document_number(DocumentNumber::from_canonical(value.0));
    }
}
