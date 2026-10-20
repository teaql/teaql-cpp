use std::io::{self, BufRead};
use serde_json::{Value as J, json};
use teaql_core::{Expr, SelectQuery};
use teaql_data_service::{DataServiceExecutor, DataServiceCapabilities, QueryExecutor, QueryRequest, QueryResult, SqlIntentRedactions};
#[path="../rust_values/src/fixture_value.rs"] mod fixture_value;
#[path="../rust_values/src/fixture_sql.rs"] mod fixture_sql;
#[path="../rust_values/src/fixture_query_keys.rs"] mod fixture_query_keys;
struct NoIo;
impl DataServiceExecutor for NoIo {
    type Error = std::io::Error;
    fn capabilities(&self) -> DataServiceCapabilities { Default::default() }
}
impl QueryExecutor for NoIo {
    async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
        panic!("intent discovery must not execute a query")
    }
}
fn observe(input: J) -> J {
    if input["kind"] == "field" {
        let entity=fixture_sql::entity(&input["entity"]);
        let policy=SqlIntentRedactions::field_policy(&entity,input["field"].as_str().unwrap());
        return json!({"policy":format!("{policy:?}")});
    }
    let query = if input["kind"] == "query" {
        fixture_query_keys::query(&input["query"])
    } else {
        let original=input["original"].as_str().unwrap();
        let mode=input["mode"].as_str().unwrap();
        let mut expr=Expr::begin_with("name",original);
        let Expr::Binary { op, right, .. } = &mut expr else { unreachable!() };
        let Expr::LikePattern { pattern, .. }=right.as_mut() else { unreachable!() };
        match mode {
            "prefix"=>{},
            "suffix"=>*pattern=format!("%{original}"),
            "contain"=>*pattern=format!("%{original}%"),
            "changed_pattern"=>*pattern="CURRENT-PATTERN%".into(),
            "changed_operator"=>*op=teaql_core::BinaryOp::Eq,
            "not_like"=>*op=teaql_core::BinaryOp::NotLike,
            "not_like_changed"=>{*op=teaql_core::BinaryOp::NotLike;*pattern="CURRENT-PATTERN%".into();},
            _=>unreachable!(),
        }
        SelectQuery::new("Root").filter(expr)
    };
    let output=NoIo.query_log_intent(&query);
    let mut safe=vec![];let mut debug=vec![];
    output.extend_secrets(false,&mut safe);output.extend_secrets(true,&mut debug);
    let mut cloned=output.clone();cloned.clear();
    json!({"safe":safe,"debug":debug,"description":format!("{output:?}"),
        "pretty":format!("{output:#?}"),"empty":output.is_empty(),"cleared_empty":cloned.is_empty()})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));}}
