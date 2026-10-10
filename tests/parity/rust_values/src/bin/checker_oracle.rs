//! Original public Checker/TypedEntityChecker/Context/module entry points.
use serde_json::{Value as J, json};
use std::{
    collections::{BTreeMap, BTreeSet},
    io::{self, BufRead},
    sync::{Arc, Mutex},
};
use teaql_core::{
    CompactRow, DataType, Entity, EntityDescriptor, EntityError, MutationValues,
    PropertyDescriptor, TeaqlEntity, Value,
};
use teaql_runtime::{
    CheckObjectStatus, CheckResult, CheckResults, Checker, EntityValues, InMemoryCheckerRegistry,
    JsonFieldNamingProfile, Language, ObjectLocation, RuntimeModule, TypedChecker,
    TypedEntityChecker, UserContext, clear_entity_status, mark_entity_status,
};
#[path = "../fixture_value.rs"]
mod fixture_value;
#[derive(Clone)]
struct Item {
    id: Value,
    version: Value,
    name: String,
    score: i64,
    loaded: Option<BTreeSet<String>>,
    dirty: Option<BTreeSet<String>>,
    boundary: Option<MutationValues>,
    omit_name: bool,
    null_name: bool,
}
impl TeaqlEntity for Item {
    const ENTITY_NAME: &'static str = "Item";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new("Item")
            .property(PropertyDescriptor::new("id", DataType::U64).id().not_null())
            .property(
                PropertyDescriptor::new("version", DataType::I64)
                    .version()
                    .not_null(),
            )
            .property(PropertyDescriptor::new("name", DataType::Text).not_null())
            .property(PropertyDescriptor::new("score", DataType::I64).not_null())
            .property(PropertyDescriptor::new("optional", DataType::Text))
    }
}
impl Entity for Item {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        let name = match row.get("name") {
            None | Some(Value::Null) => String::new(),
            Some(value) => value
                .try_text()
                .ok_or_else(|| EntityError::new("Item", "name must be Text"))?
                .to_owned(),
        };
        let score = match row.get("score") {
            None | Some(Value::Null) => 0,
            Some(value) => value
                .try_i64()
                .ok_or_else(|| EntityError::new("Item", "score must be I64"))?,
        };
        Ok(Self {
            id: row.get("id").cloned().unwrap_or(Value::Null),
            version: row.get("version").cloned().unwrap_or(Value::Null),
            name,
            score,
            loaded: None,
            dirty: None,
            boundary: None,
            omit_name: false,
            null_name: false,
        })
    }
    fn set_checker_loaded_fields(&mut self, fields: BTreeSet<String>) {
        self.loaded = Some(fields);
    }
    fn set_checker_dirty_fields(&mut self, fields: BTreeSet<String>, values: &MutationValues) {
        self.dirty = Some(fields);
        self.boundary = Some(values.clone());
    }
    fn is_field_loaded(&self, field: &str) -> bool {
        self.loaded.as_ref().is_none_or(|v| v.contains(field))
    }
    fn dirty_fields(&self) -> Option<BTreeSet<String>> {
        self.dirty.clone()
    }
    fn into_values(self) -> MutationValues {
        let mut values: BTreeMap<String, Value> = [
            ("id".into(), self.id),
            ("version".into(), self.version),
            ("score".into(), Value::I64(self.score)),
        ]
        .into();
        if !self.omit_name {
            values.insert(
                "name".into(),
                if self.null_name {
                    Value::Null
                } else {
                    self.name.into()
                },
            );
        }
        for (key, fields) in [
            ("_loaded_fields", self.loaded),
            ("_dirty_fields", self.dirty),
        ] {
            if let Some(fields) = fields {
                values.insert(
                    key.into(),
                    Value::List(fields.into_iter().map(Value::Text).collect()),
                );
            }
        }
        values.into()
    }
}
struct Fix {
    action: String,
    calls: Arc<Mutex<Vec<J>>>,
}
impl TypedChecker<Item> for Fix {
    fn check_and_fix_typed(
        &self,
        context: &UserContext,
        item: &mut Item,
        status: CheckObjectStatus,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        let boundary = item.boundary.as_ref().map(|v| {
            let record: BTreeMap<String, Value> = v.clone().into();
            Value::Object(record).to_json_value()
        });
        self.calls.lock().unwrap().push(json!({"status":status.as_str(), "loaded":item.loaded, "dirty":item.dirty,
            "boundary":boundary, "name":item.name, "score":item.score,"language":context.language().code()}));
        match self.action.as_str() {
            "fill" | "fill_and_report" => item.name = "fixed".into(),
            "increment" => item.score += 1,
            "set_id" => item.id = Value::U64(7),
            "remove_name" => item.omit_name = true,
            "null_name" => item.null_name = true,
            "dirty" => {
                item.dirty.get_or_insert_default().insert("name".into());
            }
            _ => {}
        }
        if matches!(self.action.as_str(), "report" | "fill_and_report") {
            results.push(CheckResult::required(location.clone().member("name")));
        }
    }
}
struct Plain {
    action: String,
    calls: Arc<Mutex<usize>>,
}
impl Checker for Plain {
    fn entity(&self) -> &str {
        "Item"
    }
    fn check_and_fix(
        &self,
        _: &UserContext,
        values: &mut EntityValues,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        *self.calls.lock().unwrap() += 1;
        if self.action == "fill" {
            values.insert("name".into(), "fixed".into());
        }
        if self.action == "set_id" {
            values.insert("id".into(), Value::U64(7));
        }
        if self.action == "report" || self.action == "report_twice" {
            results.push(CheckResult::required(location.clone().member("name")));
            if self.action == "report_twice" {
                results.push(results.last().unwrap().clone());
            }
        }
    }
}
fn location(input: &J) -> ObjectLocation {
    let mut out = ObjectLocation::root();
    if let Some(parts) = input["path"].as_array() {
        for part in parts {
            out = if let Some(name) = part.as_str() {
                out.member(name)
            } else {
                out.element(part.as_u64().unwrap() as usize)
            };
        }
    }
    out
}
fn describe(values: &EntityValues) -> J {
    json!(
        values
            .iter()
            .map(|(k, v)| (k.clone(), format!("{v:?}")))
            .collect::<BTreeMap<_, _>>()
    )
}
fn checks_json(checks: &[CheckResult]) -> J {
    json!(
        checks
            .iter()
            .map(|v| v.to_wire(JsonFieldNamingProfile::CamelCase))
            .collect::<Vec<_>>()
    )
}
fn evaluate(input: J) -> J {
    let mut values: EntityValues = input["values"]
        .as_object()
        .unwrap()
        .iter()
        .map(|(k, v)| (k.clone(), fixture_value::value(v)))
        .collect::<BTreeMap<_, _>>()
        .into();
    let op = input["op"].as_str().unwrap();
    if op == "status" {
        let before = CheckObjectStatus::from_values(&values).as_str();
        if let Some(status) = input["mark"].as_u64() {
            mark_entity_status(
                &mut values,
                [
                    CheckObjectStatus::Create,
                    CheckObjectStatus::Update,
                    CheckObjectStatus::Unknown,
                ][status as usize],
            );
        }
        let marked = describe(&values);
        let after = CheckObjectStatus::from_values(&values).as_str();
        clear_entity_status(&mut values);
        return json!({"before":before,"marked":marked,"after":after,"cleared":describe(&values)});
    }
    let calls = Arc::new(Mutex::new(Vec::new()));
    let plain_calls = Arc::new(Mutex::new(0));
    let action = input["action"].as_str().unwrap_or("none").to_owned();
    let plain = Plain {
        action: action.clone(),
        calls: plain_calls.clone(),
    };
    let mut context = UserContext::new()
        .with_language(Language::from_code(input["language"].as_str().unwrap_or("en")).unwrap());
    let path = location(&input);
    if op == "helpers" {
        let mut checks = vec![];
        let field = input["field"].as_str().unwrap();
        plain.required(&values, field, &path, &mut checks);
        plain.min_string_length(
            &values,
            field,
            input["min"].as_u64().unwrap() as usize,
            &path,
            &mut checks,
        );
        plain.max_string_length(
            &values,
            field,
            input["max"].as_u64().unwrap() as usize,
            &path,
            &mut checks,
        );
        return json!({"checks":checks_json(&checks)});
    }
    let typed = TypedEntityChecker::<Item, _>::new(Fix {
        action,
        calls: calls.clone(),
    });
    let mut out;
    if op == "typed" {
        let mut checks = vec![];
        typed.check_and_fix(&context, &mut values, &path, &mut checks);
        out = json!({"checks":checks_json(&checks)});
    } else {
        let is_typed = input["typed"].as_bool().unwrap_or(false);
        let mut module = RuntimeModule::new();
        if input["metadata"].as_bool().unwrap_or(false) {
            module = module.descriptor(Item::entity_descriptor());
        }
        if op == "module" {
            module = if is_typed {
                module.checker(typed)
            } else {
                module.checker(plain)
            };
            if let Some(replace) = input["replace"].as_str() {
                module = module.checker(Plain {
                    action: replace.into(),
                    calls: Arc::new(Mutex::new(0)),
                });
            }
            module.clone().apply_to(&mut context);
        } else {
            let mut registry = InMemoryCheckerRegistry::new();
            if input["registered"].as_bool().unwrap_or(false) {
                if is_typed {
                    registry.register(typed)
                } else {
                    registry = registry.with_checker(plain)
                }
            }
            context.set_checker_registry(registry);
            context.set_metadata(module.metadata);
        }
        let entity = input["entity"].as_str().unwrap_or("Item");
        let has = context.has_checker(entity);
        let result = context.check_and_fix_values_at(entity, &mut values, &path);
        let (error, checks) = match result {
            Ok(()) => (None, json!([])),
            Err(error) => {
                let checks = if let teaql_runtime::RuntimeError::Check(ref checks) = error {
                    checks_json(checks)
                } else {
                    J::Null
                };
                (Some(error.to_string()), checks)
            }
        };
        out = json!({"has":has,"error":error,"checks":checks});
    }
    out["values"] = describe(&values);
    out["seen"] = json!(*calls.lock().unwrap());
    out["plain_calls"] = json!(*plain_calls.lock().unwrap());
    out
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
