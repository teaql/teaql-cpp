
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/line
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
#[teaql(entity = "Line", table = "line_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Line {
#[teaql(id)]
    id: u64,

// @source dynamic_json.xml:9
    quantity: i64,
#[teaql(version)]
    version: i64,
// @source dynamic_json.xml:9
#[teaql(column = "\"order\"")]
    order_id: u64,

// @source dynamic_json.xml:9
#[teaql(column = "product")]
    product_id: u64,
// @source dynamic_json.xml:9
#[teaql(relation(target = "Order", local_key = "order_id", foreign_key = "id"))]
    order: Option<Box<crate::Order>>,

// @source dynamic_json.xml:9
#[teaql(relation(target = "Product", local_key = "product_id", foreign_key = "id"))]
    product: Option<Box<crate::Product>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Line {
    pub const ENTITY_NAME: &'static str = "line";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "28fa1aa5d20bee2e9a16e8a1cae0298c9eb6e64ada42eef353d25c25d1733731";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("order", 1),
        ("product", 2),
        ("quantity", 3),
        ("version", 4),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("order", "order_id", "\\\"order\\\""),
        ("product", "product_id", "product"),
        ("quantity", "quantity", "quantity"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            quantity: 0_i64,
            version: 0_i64,
            order_id: 0_u64,
            product_id: 0_u64,
            order: None,
            product: None,
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
        if let Some(entity) = &mut self.order {
            entity.attach_runtime_state_recursive(root.clone());
        }
        if let Some(entity) = &mut self.product {
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


    pub fn quantity(&self) -> i64 {
        self.changed_quantity().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.quantity)
    }

    pub fn update_quantity(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.quantity = value.try_i64().map(|value| value as i64).unwrap_or(self.quantity.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "quantity", value);
        self.__load_state.mark_loaded("quantity").expect("generated field layout");
        self
    }

    pub fn changed_quantity(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "quantity")
    }

    pub fn eval_quantity(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("quantity") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "quantity".to_string(), attempted_path: "quantity".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.quantity())
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

    pub fn order_id(&self) -> u64 {
        self.changed_order_id().and_then(|value| value.try_u64()).unwrap_or(self.order_id)
    }

    pub fn update_order_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.order_id = value.try_u64().unwrap_or(self.order_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "order_id", value);
        self.__load_state.mark_loaded("order_id").expect("generated field layout");
        self
    }

    pub fn changed_order_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "order_id")
    }

    pub fn eval_order_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("order_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "order_id".to_string(), attempted_path: "order_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.order_id())
                }}

    pub fn product_id(&self) -> u64 {
        self.changed_product_id().and_then(|value| value.try_u64()).unwrap_or(self.product_id)
    }

    pub fn update_product_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.product_id = value.try_u64().unwrap_or(self.product_id.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "product_id", value);
        self.__load_state.mark_loaded("product_id").expect("generated field layout");
        self
    }

    pub fn changed_product_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "product_id")
    }

    pub fn eval_product_id(&self) -> teaql_core::eval::EvalResult<u64> {
        if !self.is_loaded("product_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "product_id".to_string(), attempted_path: "product_id".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.product_id())
                }}
    pub fn order(&self) -> Option<&crate::Order> {
        self.order.as_deref().or_else(|| {
            if !self.is_loaded("order") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Order>(
                Self::ENTITY_NAME, self.id(), "order",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.order_id())})
    }

    pub fn eval_order(&self) -> teaql_core::eval::EvalResult<&crate::Order> {
        match self.order() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "order".to_string(), attempted_path: "order".to_string() },
        }
    }

    pub fn product(&self) -> Option<&crate::Product> {
        self.product.as_deref().or_else(|| {
            if !self.is_loaded("product") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Product>(
                Self::ENTITY_NAME, self.id(), "product",
            ) {
                return edge.as_ref();
            }
            self.__teaql_runtime_state().resolve_entity(self.product_id())})
    }

    pub fn eval_product(&self) -> teaql_core::eval::EvalResult<&crate::Product> {
        match self.product() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "product".to_string(), attempted_path: "product".to_string() },
        }
    }

}

