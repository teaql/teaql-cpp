
// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/record
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
#[teaql(entity = "Record", table = "record_data", data_service = "sqlite", audit_mask_fields = "")]
pub struct Record {
#[teaql(id)]
    id: u64,

// @source model.xml:5
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    class: rust_decimal::Decimal,

// @source model.xml:5
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    operator: rust_decimal::Decimal,

// @source model.xml:5
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    template: rust_decimal::Decimal,

// @source model.xml:5
#[teaql(column = "type")]
    type_field: i64,

// @source model.xml:5
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
#[teaql(column = "match")]
    match_field: rust_decimal::Decimal,

// @source model.xml:5
#[teaql(numeric_precision = 19)]
#[teaql(numeric_scale = 7)]
    ordinary: rust_decimal::Decimal,
#[teaql(version)]
    version: i64,
// @source model.xml:5
#[teaql(column = "platform")]
    platform_id: u64,

// @source model.xml:5
#[teaql(column = "namespace")]
    namespace_id: Option<u64>,
// @source model.xml:5
#[teaql(relation(target = "Platform", local_key = "platform_id", foreign_key = "id"))]
    platform: Option<Box<crate::Platform>>,

// @source model.xml:5
#[teaql(relation(target = "Platform", local_key = "namespace_id", foreign_key = "id"))]
    namespace: Option<Box<crate::Platform>>,
    #[teaql(dynamic)]
    dynamic: BTreeMap<String, teaql_core::Value>,
    #[teaql(skip)]
    pub __load_state: teaql_core::eval::LoadState,
}

impl Record {
    pub const ENTITY_NAME: &'static str = "record";

    // Framework metadata: complete fixed layout, including overflow slots.
    pub const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "4a592bddd1aec2975134d4d1d4258b6c740d356952bfeadffbf22b22ac2c846b";
    pub const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] = &[
        ("id", 0),
        ("platform", 1),
        ("class", 2),
        ("operator", 3),
        ("template", 4),
        ("type", 5),
        ("match", 6),
        ("ordinary", 7),
        ("namespace", 8),
        ("version", 9),
    ];
    pub const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("platform", "platform_id", "platform"),
        ("class", "class", "class"),
        ("operator", "operator", "operator"),
        ("template", "template", "template"),
        ("type", "type_field", "type"),
        ("match", "match_field", "match"),
        ("ordinary", "ordinary", "ordinary"),
        ("namespace", "namespace_id", "namespace"),
        ("version", "version", "version"),
    ];

    pub fn with_id(id: u64) -> teaql_core::Value {
        teaql_core::Value::U64(id)
    }

    pub(crate) fn runtime_new(root: teaql_runtime::EntityRuntimeState) -> Self {
        Self {
            id: 0_u64,
            class: rust_decimal::Decimal::ZERO,
            operator: rust_decimal::Decimal::ZERO,
            template: rust_decimal::Decimal::ZERO,
            type_field: 0_i64,
            match_field: rust_decimal::Decimal::ZERO,
            ordinary: rust_decimal::Decimal::ZERO,
            version: 0_i64,
            platform_id: 0_u64,
            namespace_id: None,
            platform: None,
            namespace: None,
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
        if let Some(entity) = &mut self.namespace {
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


    pub fn class(&self) -> rust_decimal::Decimal {
        self.changed_class().and_then(|value| value.try_decimal()).unwrap_or(self.class)
    }

    pub fn update_class(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.class = value.try_decimal().unwrap_or(self.class.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "class", value);
        self.__load_state.mark_loaded("class").expect("generated field layout");
        self
    }

    pub fn changed_class(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "class")
    }

    pub fn eval_class(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("class") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "class".to_string(), attempted_path: "class".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.class())
                }}


    pub fn operator(&self) -> rust_decimal::Decimal {
        self.changed_operator().and_then(|value| value.try_decimal()).unwrap_or(self.operator)
    }

    pub fn update_operator(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.operator = value.try_decimal().unwrap_or(self.operator.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "operator", value);
        self.__load_state.mark_loaded("operator").expect("generated field layout");
        self
    }

    pub fn changed_operator(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "operator")
    }

    pub fn eval_operator(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("operator") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "operator".to_string(), attempted_path: "operator".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.operator())
                }}


    pub fn template(&self) -> rust_decimal::Decimal {
        self.changed_template().and_then(|value| value.try_decimal()).unwrap_or(self.template)
    }

    pub fn update_template(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.template = value.try_decimal().unwrap_or(self.template.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "template", value);
        self.__load_state.mark_loaded("template").expect("generated field layout");
        self
    }

    pub fn changed_template(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "template")
    }

    pub fn eval_template(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("template") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "template".to_string(), attempted_path: "template".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.template())
                }}


    pub fn type_field(&self) -> i64 {
        self.changed_type_field().and_then(|value| value.try_i64()).map(|value| value as i64).unwrap_or(self.type_field)
    }

    pub fn update_type_field(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.type_field = value.try_i64().map(|value| value as i64).unwrap_or(self.type_field.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "type", value);
        self.__load_state.mark_loaded("type").expect("generated field layout");
        self
    }

    pub fn changed_type_field(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "type")
    }

    pub fn eval_type_field(&self) -> teaql_core::eval::EvalResult<i64> {
        if !self.is_loaded("type") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "type".to_string(), attempted_path: "type".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.type_field())
                }}


    pub fn match_field(&self) -> rust_decimal::Decimal {
        self.changed_match_field().and_then(|value| value.try_decimal()).unwrap_or(self.match_field)
    }

    pub fn update_match_field(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.match_field = value.try_decimal().unwrap_or(self.match_field.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "match", value);
        self.__load_state.mark_loaded("match").expect("generated field layout");
        self
    }

    pub fn changed_match_field(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "match")
    }

    pub fn eval_match_field(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("match") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "match".to_string(), attempted_path: "match".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.match_field())
                }}


    pub fn ordinary(&self) -> rust_decimal::Decimal {
        self.changed_ordinary().and_then(|value| value.try_decimal()).unwrap_or(self.ordinary)
    }

    pub fn update_ordinary(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.ordinary = value.try_decimal().unwrap_or(self.ordinary.clone());
        self.__teaql_runtime_state().set(self.entity_key(), "ordinary", value);
        self.__load_state.mark_loaded("ordinary").expect("generated field layout");
        self
    }

    pub fn changed_ordinary(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "ordinary")
    }

    pub fn eval_ordinary(&self) -> teaql_core::eval::EvalResult<rust_decimal::Decimal> {
        if !self.is_loaded("ordinary") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "ordinary".to_string(), attempted_path: "ordinary".to_string() }
                } else {
                    teaql_core::eval::EvalResult::Value(self.ordinary())
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

    pub fn namespace_id(&self) -> Option<u64> {
        self.namespace_id.clone()
    }

    pub fn update_namespace_id(&mut self, value: impl Into<teaql_core::Value>) -> &mut Self {
        let value = value.into();
        self.namespace_id = if matches!(value, teaql_core::Value::Null) { None } else { value.try_u64().map(Some).unwrap_or_else(|| self.namespace_id.clone()) };
        self.__teaql_runtime_state().set(self.entity_key(), "namespace_id", value);
        self.__load_state.mark_loaded("namespace_id").expect("generated field layout");
        self
    }

    pub fn changed_namespace_id(&self) -> Option<teaql_core::Value> {
        self.__teaql_runtime_state().get(&self.entity_key(), "namespace_id")
    }

    pub fn eval_namespace_id(&self) -> teaql_core::eval::EvalResult<Option<u64>> {
        if !self.is_loaded("namespace_id") {
                    teaql_core::eval::EvalResult::NotLoaded { failed_node: "namespace_id".to_string(), attempted_path: "namespace_id".to_string() }
                } else {
                    match &self.namespace_id {
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

    pub fn namespace(&self) -> Option<&crate::Platform> {
        self.namespace.as_deref().or_else(|| {
            if !self.is_loaded("namespace") {
                return None;
            }
            if let Some(edge) = self.__teaql_runtime_state().resolve_relation_option::<crate::Platform>(
                Self::ENTITY_NAME, self.id(), "namespace",
            ) {
                return edge.as_ref();
            }
            self.namespace_id()
                            .and_then(|id| self.__teaql_runtime_state().resolve_entity(id))})
    }

    pub fn eval_namespace(&self) -> teaql_core::eval::EvalResult<&crate::Platform> {
        match self.namespace() {
            Some(v) => teaql_core::eval::EvalResult::Value(v),
            None if self.is_loaded("namespace_id") && self.namespace_id().is_none() => teaql_core::eval::EvalResult::Null,
            None => teaql_core::eval::EvalResult::NotLoaded { failed_node: "namespace".to_string(), attempted_path: "namespace".to_string() },
        }
    }

}

