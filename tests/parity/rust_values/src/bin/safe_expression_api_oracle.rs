//! Original SafeExpression calls and observable callback order; no copied evaluators.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use std::sync::atomic::{AtomicUsize, Ordering};
use teaql_core::{
    BaseEntity, BaseEntityData, CompactRow, Entity, EntityDescriptor, EntityError, MutationValues,
    SafeExpression, SmartList, TeaqlEmpty, TeaqlEntity, Value,
};
#[path = "../fixture_value.rs"]
mod fixture_value;

fn shown(value: Option<Value>) -> Json {
    json!(value.map(|v| format!("{v:?}")))
}

struct Owned(String);

struct EmptyFlag {
    empty: bool,
    predicates: Arc<AtomicUsize>,
}
impl TeaqlEmpty for EmptyFlag {
    fn teaql_is_empty(&self) -> bool {
        self.predicates.fetch_add(1, Ordering::SeqCst);
        self.empty
    }
}
fn custom_empty(input: &Json) -> Json {
    let evaluations = Arc::new(AtomicUsize::new(0));
    let predicates = Arc::new(AtomicUsize::new(0));
    let evaluating = evaluations.clone();
    let evaluating_predicates = predicates.clone();
    let present = input["present"].as_bool().unwrap();
    let empty = input["empty"].as_bool().unwrap();
    let expression = SafeExpression::new(7, move |_| {
        evaluating.fetch_add(1, Ordering::SeqCst);
        present.then(|| EmptyFlag { empty, predicates: evaluating_predicates.clone() })
    });
    let is_empty = expression.is_empty();
    let is_not_empty = expression.is_not_empty();
    let mut empty_callbacks = 0;
    let mut nonempty_callbacks = 0;
    expression.when_is_empty(|| empty_callbacks += 1);
    expression.when_not_empty(|_| nonempty_callbacks += 1);
    json!({"empty":is_empty,"not_empty":is_not_empty,
        "evaluations":evaluations.load(Ordering::SeqCst),
        "predicates":predicates.load(Ordering::SeqCst),
        "empty_callbacks":empty_callbacks,"nonempty_callbacks":nonempty_callbacks})
}

fn owned(input: &Json) -> Json {
    let root = Owned(input["root"].as_str().unwrap().to_owned());
    let captured = Owned(input["capture"].as_str().unwrap().to_owned());
    let expression = SafeExpression::new(root, move |root| {
        Some(Owned(format!("{}{}", root.0, captured.0)))
    });
    let initial = expression.eval().unwrap().0;
    let mapped = expression
        .apply(|v| Owned(format!("{}-mapped", v.0)))
        .apply_optional(|v| Some(v));
    let result = mapped.eval().unwrap().0;
    let eager = mapped.or_else(Owned("unused".into())).0;
    let absent = SafeExpression::new(7, |_| None::<Owned>);
    let fallback = absent.or_else_with(|| Owned("fallback".into())).0;
    let error = absent
        .or_else_throw(|| Owned("error".into()))
        .err()
        .unwrap()
        .0;
    json!({"initial":initial,"mapped":result,"eager":eager,"fallback":fallback,"error":error})
}

#[derive(Clone)]
struct Item(BaseEntityData);

impl TeaqlEntity for Item {
    const ENTITY_NAME: &'static str = "SafeItem";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new(Self::ENTITY_NAME)
    }
}
impl Entity for Item {
    fn from_compact_row(_: CompactRow) -> Result<Self, EntityError> {
        unreachable!()
    }
    fn into_values(self) -> MutationValues {
        self.0.to_values_map().into()
    }
}
impl BaseEntity for Item {
    fn base(&self) -> &BaseEntityData {
        &self.0
    }
    fn base_mut(&mut self) -> &mut BaseEntityData {
        &mut self.0
    }
}

fn entity(input: &Json) -> Json {
    let id = input["id"].as_str().unwrap().parse().unwrap();
    let version = input["version"].as_str().unwrap().parse().unwrap();
    let next = input["next"].as_str().unwrap().parse().unwrap();
    let value = Item(BaseEntityData::new().with_id(id).with_version(version));
    let expression = SafeExpression::value(value);
    let changed = expression.clone().update_entity_id(next).eval().unwrap();
    json!({"id":expression.clone().entity_id().eval().map(|v|v.to_string()),
           "version":expression.clone().entity_version().eval().map(|v|v.to_string()),
           "changed_id":changed.id().to_string(),"changed_version":changed.version_value().to_string(),
           "original_id":expression.root().id().to_string()})
}

fn observe(input: Json) -> Json {
    if input["kind"] == "custom_empty" { return custom_empty(&input); }
    if input["kind"] == "owned" {
        return owned(&input);
    }
    if input["kind"] == "entity" {
        return entity(&input);
    }
    let root = fixture_value::value(&input["root"]);
    let alternate = fixture_value::value(&input["alternate"]);
    let fallback = fixture_value::value(&input["fallback"]);
    let calls = Arc::new(Mutex::new(Vec::<String>::new()));
    let evaluating = calls.clone();
    let present = input["present"].as_bool().unwrap();
    let base = SafeExpression::new(root.clone(), move |root: &Value| {
        evaluating
            .lock()
            .unwrap()
            .push(format!("evaluate:{root:?}"));
        present.then(|| root.clone())
    });
    let first = calls.clone();
    let missing = input["mode"] == "missing";
    let second = calls.clone();
    let wrap = input["mode"] == "wrap";
    let expression = base
        .clone()
        .apply_optional(move |value| {
            first.lock().unwrap().push(format!("optional:{value:?}"));
            (!missing).then_some(value)
        })
        .apply(move |value| {
            second.lock().unwrap().push(format!("map:{value:?}"));
            if wrap {
                Value::List(vec![value])
            } else {
                value
            }
        });
    let copied = base.clone();
    let shared_root = std::ptr::eq(base.root(), copied.root());
    let eval = shown(expression.eval());
    let supplied = shown(expression.eval_with(&alternate));
    let clone_eval = shown(expression.clone().eval());
    let eager = format!("{:?}", expression.or_else(fallback.clone()));
    let lazy = format!(
        "{:?}",
        expression.or_else_with(|| {
            calls.lock().unwrap().push("fallback".into());
            fallback.clone()
        })
    );
    let outcome = expression.or_else_throw(|| {
        calls.lock().unwrap().push("error".into());
        "missing value".to_owned()
    });
    let outcome = match outcome {
        Ok(value) => json!({"ok":format!("{value:?}")}),
        Err(error) => json!({"error":error}),
    };
    let is_null = expression.is_null();
    let not_null = expression.is_not_null();
    let empty = expression.is_empty();
    let not_empty = expression.is_not_empty();
    expression.when_is_null(|| calls.lock().unwrap().push("null".into()));
    expression.when_is_not_null(|v| calls.lock().unwrap().push(format!("value:{v:?}")));
    expression.when_is_empty(|| calls.lock().unwrap().push("empty".into()));
    expression.when_not_empty(|v| calls.lock().unwrap().push(format!("nonempty:{v:?}")));
    let identity = SafeExpression::value(root.clone());
    let list = SmartList::new(
        input["list"]
            .as_array()
            .unwrap()
            .iter()
            .map(fixture_value::value)
            .collect(),
    );
    let list_expression = SafeExpression::value(list);
    json!({"eval":eval,"supplied":supplied,"clone_eval":clone_eval,"eager":eager,"lazy":lazy,
           "outcome":outcome,"null":is_null,"not_null":not_null,"empty":empty,"not_empty":not_empty,
           "shared_root":shared_root,"root":format!("{:?}",identity.root()),
           "identity":shown(identity.eval()),"direct_empty":root.teaql_is_empty(),
           "none_empty":Option::<Value>::None.teaql_is_empty(),
           "some_null_empty":Some(Value::Null).teaql_is_empty(),
           "size":list_expression.clone().size().eval(),
           "first":shown(list_expression.clone().first().eval()),
           "get":shown(list_expression.get(input["index"].as_u64().unwrap() as usize).eval()),
           "calls":calls.lock().unwrap().clone()})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
