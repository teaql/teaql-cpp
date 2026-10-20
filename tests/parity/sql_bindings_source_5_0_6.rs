use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::{BinaryOp, DataType, EntityDescriptor, Expr, ExprFunction, PropertyDescriptor, SelectQuery, Value};
use teaql_data_service::{SqlExecutionOutcome, SqlIntentRedactions, SqlLogContext, SqlParameterLogPolicy as Policy};
pub use teaql_sql::DatabaseKind;
#[path = "../../.reference/teaql-rs/teaql-sql/src/bindings.rs"]
mod original;

fn policy(value: &Json) -> Option<Policy> {
    value.as_u64().map(|index| [Policy::Unknown, Policy::Plain, Policy::Masked, Policy::Credential][index as usize])
}
fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}
fn entity(mode: u64) -> EntityDescriptor {
    let root = EntityDescriptor::new("Device")
        .property(PropertyDescriptor::new("plain", DataType::Text))
        .property(PropertyDescriptor::new("masked", DataType::Text).column_name("db_masked"))
        .property(PropertyDescriptor::new("password", DataType::Text))
        .property(PropertyDescriptor::new("token_alias", DataType::Text).column_name("access_token"));
    match mode {
        0 => root,
        1 => root.audit_mask_fields(vec![]),
        2 => root.audit_mask_fields(vec!["masked".into()]),
        _ => root.audit_mask_fields(vec!["plain".into(), "password".into(), "db_masked".into()]),
    }
}
fn expression(field: &str, shape: u64) -> Expr {
    let column = Expr::column(field);
    match shape {
        0 => column,
        1 => Expr::value("literal"),
        2 => Expr::LikePattern {pattern:"%original%".into(), original:"original".into()},
        3 => Expr::function(ExprFunction::Count, []),
        4 => Expr::function(ExprFunction::Sum, [column, Expr::column("masked")]),
        5 => Expr::binary(column, BinaryOp::Eq, Expr::column("masked")),
        6 => Expr::Between {expr:Box::new(column),lower:Box::new(Expr::value(1)),upper:Box::new(Expr::column("masked"))},
        7 => Expr::IsNull(Box::new(column)),
        8 => Expr::IsNotNull(Box::new(column)),
        9 => Expr::SubQuery {left:Box::new(column),op:BinaryOp::In,entity:entity(3),
             query:Box::new(SelectQuery::new("Child").filter(Expr::eq("password", "child-secret")))},
        10 => Expr::And(vec![column, Expr::column("password")]),
        11 => Expr::Or(vec![column, Expr::column("password")]),
        12 => Expr::Not(Box::new(column)),
        _ => Expr::soundex(column),
    }
}
fn context(value: &SqlLogContext) -> Json {
    let mut safe = Vec::new(); let mut debug = Vec::new();
    value.intent_redactions.extend_secrets(false, &mut safe);
    value.intent_redactions.extend_secrets(true, &mut debug);
    json!({"formatted":formatted(value),"safe_secrets":safe,"debug_secrets":debug})
}
fn observe(input: &Json) -> Json {
    match input["kind"].as_str().unwrap() {
        "combine" => json!(original::combine(policy(&input["left"]),policy(&input["right"])).map(|p|format!("{p:?}"))),
        "expression" => {
            let root = entity(input["mode"].as_u64().unwrap());
            let field = input["field"].as_str().unwrap();
            let expr = expression(field, input["shape"].as_u64().unwrap());
            json!({"field":format!("{:?}",original::field_policy(&root,field)),
                "expression":original::expression_policy(&root,&expr).map(|p|format!("{p:?}"))})
        },
        "like" => {
            let text = input["text"].as_str().unwrap();
            let variant = input["variant"].as_u64().unwrap();
            let pattern = match variant {0=>format!("%{text}%"),1=>format!("{text}%"),2=>format!("%{text}"),3=>text.into(),_=>"%changed%".into()};
            let right = match variant {4=>Expr::value(pattern),5=>Expr::soundex(Expr::value(pattern)),
                _=>Expr::LikePattern {pattern,original:text.into()}};
            let op = [BinaryOp::Eq,BinaryOp::Ne,BinaryOp::Gt,BinaryOp::Gte,BinaryOp::Lt,BinaryOp::Lte,
                BinaryOp::Like,BinaryOp::NotLike,BinaryOp::In,BinaryOp::NotIn,BinaryOp::InLarge,BinaryOp::NotInLarge][input["op"].as_u64().unwrap() as usize];
            json!(original::like_operand(op,&right))
        },
        "bindings" => {
            let root = entity(input["mode"].as_u64().unwrap());
            let text = input["text"].as_str().unwrap();
            let mut values = original::SqlBindings::new(); values.scope=policy(&input["scope"]);
            values.push(Value::from("generic"));
            values.push_like_pattern(&format!("%{text}%"),text);
            values.push_field(&root,"plain",Value::from("PLAIN"));
            let previous = values.scope; values.scope=original::combine(previous,Some(Policy::Credential));
            let second = format!("{text}-private");
            values.push_like_pattern(&format!("{second}%"),&second);
            values.push_field(&root,"masked",Value::from("MASKED")); values.scope=previous;
            values.push_like_pattern(&format!("%{text}"),text);
            if input["untrusted"]==true { values.mark_untrusted_sql(); }
            let current = formatted(&values);
            let log = context(&values.log_context(DatabaseKind::Sqlite));
            let mut source = SqlLogContext {database_kind:Some("cached\0source".into()),
                parameter_policies:input["source_policies"].as_array().unwrap().iter().map(|x|policy(x).unwrap()).collect(),
                generated_sql:input["source_generated"].as_bool().unwrap(), masked_parameters:vec![true,false,true],
                execution_outcome:Some(SqlExecutionOutcome::Cancelled),mode:Some("source-mode".into()),
                omission_reason:Some("source-omission".into()),..Default::default()};
            source.intent_redactions=SqlIntentRedactions::from_unclassified_query(&SelectQuery::new("Old").filter(Expr::eq("password","STALE-PRIVATE")));
            let before=context(&source); let rebound=context(&values.rebind_log_context(&source));
            let after=context(&source);
            let mut clone=values.clone(); clone.push(Value::from("clone-only"));
            json!({"bindings":current,"log":log,"source_before":before,"source_after":after,"rebound":rebound,
                "clone":formatted(&clone),"values":values.into_values().iter().map(|v|format!("{v:?}")).collect::<Vec<_>>()})
        },
        _ => panic!("Unknown binding fixture"),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
