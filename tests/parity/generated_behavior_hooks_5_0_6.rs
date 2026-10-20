use serde_json::{json, Value as Json};
use std::{io::{self, BufRead}, sync::{Arc, Mutex}};
use teaql_core::{DeleteCommand, InsertCommand, RecoverCommand, SelectQuery, UpdateCommand};
use teaql_runtime::{EntityDataServiceBehavior, RuntimeError, RuntimeModule, UserContext};

struct Settings { text: String, hooks: Arc<Mutex<Vec<String>>> }
struct Custom<B> { base: B, mode: String, prefix: String }
impl<B: EntityDataServiceBehavior> Custom<B> {
    fn apply(&self, user_context: &UserContext, hook: &str, entity: &mut String) -> Result<(), RuntimeError> {
        let settings = user_context.require_resource::<Settings>().unwrap();
        settings.hooks.lock().unwrap().push(hook.into());
        entity.push_str(&self.prefix);
        entity.push_str(&settings.text);
        if self.mode == "deny" { Err(RuntimeError::Graph("business behavior denied".into())) } else { Ok(()) }
    }
}
macro_rules! hook {
    ($method:ident, $command:ty, $label:literal) => {
        fn $method(&self, user_context: &UserContext, command: &mut $command) -> Result<(), RuntimeError> {
            self.base.$method(user_context, command)?;
            self.apply(user_context, $label, &mut command.entity)
        }
    };
}
impl<B: EntityDataServiceBehavior> EntityDataServiceBehavior for Custom<B> {
    hook!(before_select, SelectQuery, "select");
    hook!(before_insert, InsertCommand, "insert");
    hook!(before_update, UpdateCommand, "update");
    hook!(before_delete, DeleteCommand, "delete");
    hook!(before_recover, RecoverCommand, "recover");
    fn relation_loads(&self, user_context: &UserContext) -> Vec<String> {
        let settings = user_context.require_resource::<Settings>().unwrap();
        settings.hooks.lock().unwrap().push("relations".into());
        let mut paths = self.base.relation_loads(user_context);
        paths.push(format!("{}{}", self.prefix, settings.text));
        paths
    }
}

fn observe(input: &Json) -> Json {
    let mode = input["mode"].as_str().unwrap();
    let replacement = input["replacement"] == true;
    macro_rules! module {
        ($domain:ident, $model:ty, $behavior:expr) => {{
            let module: RuntimeModule = match input["module"].as_str().unwrap() {
                "plain" => $domain::module(),
                "checker" => $domain::module_with_checkers(),
                "behavior" => $domain::module_with_behaviors(),
                _ => $domain::module_with_behaviors_and_checkers(),
            };
            if mode == "default" { module } else {
                let module = module.entity_with_behavior::<$model, _>(Custom {
                    base: $behavior, mode: mode.into(), prefix: "first:".into(),
                });
                if replacement {
                    module.entity_with_behavior::<$model, _>(Custom {
                        base: $behavior, mode: mode.into(), prefix: "next:".into(),
                    })
                } else { module }
            }
        }};
    }
    use cpp_checker_fixture_core as notes;
    use cpp_inheritance_variants_core as inherited;
    use cpp_constants_numeric_core as constants;
    let (module, name) = match input["model"].as_str().unwrap() {
        "note" => (module!(notes, notes::Note, notes::NoteBehavior), "Note"),
        "invoice" => (module!(inherited, inherited::Invoice, inherited::InvoiceBehavior), "Invoice"),
        _ => (module!(constants, constants::Priority, constants::PriorityBehavior), "Priority"),
    };
    let mut user_context = module.into_context();
    let hooks = Arc::new(Mutex::new(Vec::new()));
    user_context.insert_resource(Settings { text: input["text"].as_str().unwrap().into(), hooks: hooks.clone() });
    let behavior = user_context.entity_data_service_behavior(name);
    let present = behavior.is_some();
    let operation = input["operation"].as_str().unwrap();
    let mut command_text = Json::Null;
    let mut paths = Vec::new();
    let mut error = None;
    macro_rules! invoke {
        ($command:expr, $hook:ident) => {{
            let mut command = $command;
            if let Some(behavior) = behavior.as_ref() {
                error = behavior.$hook(&user_context, &mut command).err().map(|e|e.to_string());
            }
            command_text = json!(format!("{command:?}"));
        }};
    }
    match operation {
        "select" => invoke!(SelectQuery::new(name), before_select),
        "insert" => invoke!(InsertCommand::new(name).value("name", input["text"].as_str().unwrap()), before_insert),
        "update" => invoke!(UpdateCommand::new(name, 7_u64).value("name", input["text"].as_str().unwrap()), before_update),
        "delete" => invoke!(DeleteCommand::new(name, 7_u64), before_delete),
        "recover" => invoke!(RecoverCommand::new(name, 7_u64, -3), before_recover),
        _ => { if let Some(behavior) = behavior.as_ref() { paths = behavior.relation_loads(&user_context); } }
    }
    json!({"present":present,"command":command_text,"paths":paths,"error":error,"hooks":hooks.lock().unwrap().clone()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
