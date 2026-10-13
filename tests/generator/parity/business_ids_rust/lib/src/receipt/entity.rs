
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/receipt
use std::collections::BTreeMap;

use teaql_macros::{teaql_entity, TeaqlEntity};

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct ReceiptNumber(String);

impl ReceiptNumber {
    pub fn new(value: impl Into<String>) -> Result<Self, &'static str> {
        let value = value.into().trim().to_owned();
        if value.is_empty() {
            return Err("receipt_number must not be blank");
        }
        Ok(Self(value))
    }

    pub(crate) fn from_canonical(value: impl Into<String>) -> Self {
        Self(value.into())
    }

    pub fn as_str(&self) -> &str { &self.0 }

    pub fn definition() -> teaql_core::business_id::BusinessIdDefinition {
        teaql_core::business_id::BusinessIdDefinition {
            field_name: "receipt_number".to_owned(),
            profile: "daily-permuted-v1".to_owned(),
            prefix: "RCP".to_owned(),
            namespace: "receipts".to_owned(),
            split_by_date: true,
            preserve_digits: 6,
            separator: "-".to_owned(),
        }
    }
}

impl From<ReceiptNumber> for teaql_core::Value {
    fn from(value: ReceiptNumber) -> Self {
        teaql_core::Value::Text(value.0)
    }
}

impl From<&ReceiptNumber> for teaql_core::Value {
    fn from(value: &ReceiptNumber) -> Self {
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
#[teaql(entity = "Receipt", table = "receipt_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Receipt {
#[teaql(id)]
    id: u64,

// @source business_ids.xml:4
#[teaql(max_length = 100)]
    name: String,

// @source business_ids.xml:4
#[teaql(max_length = 100)]
    receipt_number: String,
#[teaql(version)]
    version: i64,
// @source business_ids.xml:4
#[teaql(column = "tenant")]
    tenant_id: u64,
// @source business_ids.xml:4
#[teaql(relation(target = "Tenant", local_key = "tenant_id", foreign_key = "id"))]
    tenant: Option<Box<crate::Tenant>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Receipt {
    pub const ENTITY_NAME: &'static str = "receipt";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "bf7069df8d3f686de381b1b9d0d9f49eec0650ef77256653ccd5e4bc82e1ed48";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("name", 1),
        ("tenant", 2),
        ("receipt_number", 3),
        ("version", 4),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("name", "name", "name"),
        ("tenant", "tenant_id", "tenant"),
        ("receipt_number", "receipt_number", "receipt_number"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            name: String::new(),
            receipt_number: String::new(),
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


    pub fn receipt_number(&self) -> ReceiptNumber {
        let value = self.changed_receipt_number()
            .and_then(|value| value.try_text().map(str::to_owned))
            .unwrap_or_else(|| self.receipt_number.clone());
        ReceiptNumber::from_canonical(value)
    }

    pub fn update_receipt_number(&mut self, value: ReceiptNumber) -> &mut Self {
        let canonical = value.0.clone();
        self.receipt_number = canonical.clone();
        self.__teaql_runtime_state().set(
            self.entity_key(),
            "receipt_number",
            teaql_core::Value::Text(canonical),
        );
        self.__load_state.mark_loaded("receipt_number").expect("generated field layout");
        self
    }

    pub fn changed_receipt_number(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "receipt_number")
    }

    pub fn eval_receipt_number(&self) -> teaql_core::eval::EvalResult<ReceiptNumber> {
        if !self.is_loaded("receipt_number") {
            teaql_core::eval::EvalResult::NotLoaded {
                failed_node: "receipt_number".to_string(),
                attempted_path: "receipt_number".to_string(),
            }
        } else {
            teaql_core::eval::EvalResult::Value(self.receipt_number())
        }
    }


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

pub(crate) struct ReceiptReceiptNumberBusinessIdSlot<'a>(
    pub(crate) &'a mut Receipt,
    pub(crate) bool,
);

impl teaql_core::business_id::BusinessIdSlot for ReceiptReceiptNumberBusinessIdSlot<'_> {
    fn current_business_id(&self) -> Option<&str> {
        (!self.0.receipt_number.is_empty()).then_some(self.0.receipt_number.as_str())
    }

    fn is_new_aggregate(&self) -> bool {
        self.1
    }

    fn assign_business_id(&mut self, value: teaql_core::business_id::BusinessIdValue) {
        self.0.update_receipt_number(ReceiptNumber::from_canonical(value.0));
    }
}
