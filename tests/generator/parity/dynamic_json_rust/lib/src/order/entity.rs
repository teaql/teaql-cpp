
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/order
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
#[teaql(reverse_relation(name = "lines", target = "Line", json_type = "crate::Line", local_key = "id", foreign_key = "order_id", many))]
#[teaql(entity = "Order", table = "order_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Order {
#[teaql(id)]
    id: u64,

// @source dynamic_json.xml:7
#[teaql(max_length = 100)]
    code: String,

// @source dynamic_json.xml:7
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    amount: rust_decimal::Decimal,

// @source dynamic_json.xml:7
    priority: i64,
#[teaql(version)]
    version: i64,
// @source dynamic_json.xml:7
#[teaql(column = "platform")]
    platform_id: u64,

// @source dynamic_json.xml:7
#[teaql(column = "customer")]
    customer_id: u64,

// @source dynamic_json.xml:7
#[teaql(column = "seller")]
    seller_id: u64,
// @source dynamic_json.xml:7
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,

// @source dynamic_json.xml:7
#[teaql(relation(target = "Person", local_key = "customer_id", foreign_key = "id"))]
    customer: Option<Box<crate::Person>>,

// @source dynamic_json.xml:7
#[teaql(relation(target = "Company", local_key = "seller_id", foreign_key = "id"))]
    seller: Option<Box<crate::Company>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Order {
    pub const ENTITY_NAME: &'static str = "order";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "8fe2b4bd0d55b51a79eb8ef06e2545234d3ca9f278ec7bd7382be5c5190bc79b";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("customer", 2),
        ("seller", 3),
        ("code", 4),
        ("amount", 5),
        ("priority", 6),
        ("version", 7),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("customer", "customer_id", "customer"),
        ("seller", "seller_id", "seller"),
        ("code", "code", "code"),
        ("amount", "amount", "amount"),
        ("priority", "priority", "priority"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            code: String::new(),
            amount: rust_decimal::Decimal::ZERO,
            priority: 0_i64,
            version: 0_i64,
            platform_id: 0_u64,
            customer_id: 0_u64,
            seller_id: 0_u64,
            platform: None,
            customer: None,
            seller: None,
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
        if let Some(entity) = &mut self.customer {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.seller {
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


    pub fn amount(&self) -> rust_decimal::Decimal {
        self.changed_amount().and_then(|value| value.try_decimal()).unwrap_or(self.amount)
    }

    pub fn update_amount(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.amount = value.try_decimal().unwrap_or(self.amount.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "amount", value);
        self.__load_state.mark_loaded("amount").expect("generated field layout");
        self
    }

    pub fn changed_amount(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "amount")
    }

    pub fn eval_amount(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("amount") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "amount".to_string(), attempted_path: "amount".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.amount())
                }}


    pub fn priority(&self) -> i64 {
        self.changed_priority().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.priority)
    }

    pub fn update_priority(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.priority = value.try_i64().map(|value| value as i64).unwrap_or(self.priority.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "priority", value);
        self.__load_state.mark_loaded("priority").expect("generated field layout");
        self
    }

    pub fn changed_priority(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "priority")
    }

    pub fn eval_priority(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("priority") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "priority".to_string(), attempted_path: "priority".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.priority())
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

    pub fn customer_id(&self) -> u64 {
        self.changed_customer_id().and_then(|value| value.try_u64()).unwrap_or(self.customer_id)
    }

    pub fn update_customer_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.customer_id = value.try_u64().unwrap_or(self.customer_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "customer_id", value);
        self.__load_state.mark_loaded("customer_id").expect("generated field layout");
        self
    }

    pub fn changed_customer_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "customer_id")
    }

    pub fn eval_customer_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("customer_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "customer_id".to_string(), attempted_path: "customer_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.customer_id())
                }}

    pub fn seller_id(&self) -> u64 {
        self.changed_seller_id().and_then(|value| value.try_u64()).unwrap_or(self.seller_id)
    }

    pub fn update_seller_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.seller_id = value.try_u64().unwrap_or(self.seller_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "seller_id", value);
        self.__load_state.mark_loaded("seller_id").expect("generated field layout");
        self
    }

    pub fn changed_seller_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "seller_id")
    }

    pub fn eval_seller_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("seller_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "seller_id".to_string(), attempted_path: "seller_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.seller_id())
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

    pub fn customer(&self) -> Option<&crate::Person> {
        self.customer.as_deref().or_else(|| {
            if !self.is_loaded("customer") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Person>(
                Self::ENTITY_NAME, self.id(), "customer",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.customer_id())})
    }

    pub fn eval_customer(&self) -> teaql_core::eval::EvalResult<&crate::Person> {
        match self.customer() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "customer".to_string(), attempted_path: "customer".to_string() },
        }
    }

    pub fn seller(&self) -> Option<&crate::Company> {
        self.seller.as_deref().or_else(|| {
            if !self.is_loaded("seller") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Company>(
                Self::ENTITY_NAME, self.id(), "seller",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.seller_id())})
    }

    pub fn eval_seller(&self) -> teaql_core::eval::EvalResult<&crate::Company> {
        match self.seller() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "seller".to_string(), attempted_path: "seller".to_string() },
        }
    }
    /// Returns the relation view installed by the query that loaded this entity.
    /// This method never performs an implicit database query.
    pub fn lines(&self) -> teaql_runtime::RelationHandle<'_, teaql_core::SmartList<crate::Line>> {
        self.__teaql_runtime_state().relation_list(
            <Self as teaql_core::TeaqlEntity>::ENTITY_NAME,
            self.id(),
            "lines",
        )
    }

    pub fn eval_lines(&self) -> teaql_core::eval::EvalResult<&teaql_core::SmartList<crate::Line>> {
        let relation = self.lines();
        match relation.state() {
            teaql_runtime::LoadedRelation::Loaded | teaql_runtime::LoadedRelation::Empty => teaql_core::eval::EvalResult::Value(relation.value().expect("loaded list relation must have a value")),
            teaql_runtime::LoadedRelation::NotLoaded => teaql_core::eval::EvalResult::NotLoaded { failed_node: "lines".to_string(), attempted_path: "lines".to_string() },
        }
    }

}

