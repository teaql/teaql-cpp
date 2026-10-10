//! Direct calls to the unchanged pinned in-memory engine and evaluator.
use serde_json::{Value as J, json};
use std::io::{self, BufRead};
use teaql_core::*;
use teaql_runtime::{ExprEvaluator, InMemoryQueryEngine};
#[path = "../fixture_value.rs"]
mod fixture_value;

fn expression(j: &J) -> Expr {
    let args = || {
        j["args"]
            .as_array()
            .unwrap()
            .iter()
            .map(expression)
            .collect::<Vec<_>>()
    };
    match j["kind"].as_str().unwrap() {
        "column" => Expr::column(j["name"].as_str().unwrap()),
        "value" => Expr::Value(fixture_value::value(&j["literal"])),
        "binary" => {
            let mut a = args().into_iter();
            let op = [
                BinaryOp::Eq,
                BinaryOp::Ne,
                BinaryOp::Gt,
                BinaryOp::Gte,
                BinaryOp::Lt,
                BinaryOp::Lte,
                BinaryOp::Like,
                BinaryOp::NotLike,
                BinaryOp::In,
                BinaryOp::NotIn,
                BinaryOp::InLarge,
                BinaryOp::NotInLarge,
            ][j["op"].as_u64().unwrap() as usize];
            Expr::Binary {
                left: Box::new(a.next().unwrap()),
                op,
                right: Box::new(a.next().unwrap()),
            }
        }
        "and" => Expr::And(args()),
        "or" => Expr::Or(args()),
        "not" => Expr::Not(Box::new(args().remove(0))),
        "isnull" => Expr::IsNull(Box::new(args().remove(0))),
        "notnull" => Expr::IsNotNull(Box::new(args().remove(0))),
        "between" => {
            let mut a = args().into_iter();
            Expr::Between {
                expr: Box::new(a.next().unwrap()),
                lower: Box::new(a.next().unwrap()),
                upper: Box::new(a.next().unwrap()),
            }
        }
        "function" => Expr::function(ExprFunction::Soundex, args()),
        "subquery" => Expr::SubQuery {
            left: Box::new(Expr::column("x")),
            op: BinaryOp::In,
            entity: EntityDescriptor::new("Item"),
            query: Box::new(SelectQuery::new("Item")),
        },
        _ => panic!("unknown expression"),
    }
}
fn row(j: &J) -> CompactRow {
    let pairs = j.as_array().unwrap();
    CompactRow::new(
        pairs
            .iter()
            .map(|p| p[0].as_str().unwrap().to_owned())
            .collect::<Vec<_>>()
            .into(),
        pairs.iter().map(|p| fixture_value::value(&p[1])).collect(),
    )
}
fn evaluate(j: J) -> J {
    if j["op"] == "eval" {
        let e = expression(&j["expr"]);
        let r = row(&j["row"]);
        return json!({"eval":ExprEvaluator::eval(&e, &r), "value":format!("{:?}", ExprEvaluator::resolve(&e, &r))});
    }
    let q = &j["query"];
    let mut query = SelectQuery::new("Item");
    if let Some(expr) = q.get("filter") {
        query.filter = Some(expression(expr));
    }
    if let Some(expr) = q.get("having") {
        query.having = Some(expression(expr));
    }
    if let Some(p) = q.get("projection") {
        query.projection = p
            .as_array()
            .unwrap()
            .iter()
            .map(|s| s.as_str().unwrap().into())
            .collect();
    }
    if let Some(g) = q.get("groups") {
        query.group_by = g
            .as_array()
            .unwrap()
            .iter()
            .map(|s| s.as_str().unwrap().into())
            .collect();
    }
    if let Some(a) = q.get("aggregates") {
        query.aggregates = a
            .as_array()
            .unwrap()
            .iter()
            .map(|a| {
                Aggregate::new(
                    [
                        AggregateFunction::Count,
                        AggregateFunction::Sum,
                        AggregateFunction::Avg,
                        AggregateFunction::Min,
                        AggregateFunction::Max,
                        AggregateFunction::Stddev,
                        AggregateFunction::StddevPop,
                        AggregateFunction::VarSamp,
                        AggregateFunction::VarPop,
                        AggregateFunction::BitAnd,
                        AggregateFunction::BitOr,
                        AggregateFunction::BitXor,
                    ][a[0].as_u64().unwrap() as usize],
                    a[1].as_str().unwrap(),
                    a[2].as_str().unwrap(),
                )
            })
            .collect();
    }
    if let Some(order) = q.get("order") {
        query.order_by = order
            .as_array()
            .unwrap()
            .iter()
            .map(|o| {
                let mut ob = OrderBy::new(
                    o[0].as_str().unwrap(),
                    if o[1] == true {
                        SortDirection::Desc
                    } else {
                        SortDirection::Asc
                    },
                );
                if o.as_array().unwrap().len() > 2 {
                    ob.expr = Some(expression(&o[2]));
                }
                ob
            })
            .collect();
    }
    if let Some(s) = q.get("slice") {
        query.slice = Some(Slice {
            offset: s[0].as_u64().unwrap(),
            limit: s[1].as_u64(),
        });
    }
    if let Some(s) = q.get("comment") {
        query.comment = Some(s.as_str().unwrap().into());
    }
    if let Some(n) = q.get("hard_limit") {
        query.hard_limit = n.as_u64().unwrap();
    }
    let result = InMemoryQueryEngine::execute(
        &query,
        j["rows"].as_array().unwrap().iter().map(row).collect(),
    );
    let m = result.metadata;
    json!({"rows":result.rows.iter().map(|r| r.iter().map(|(k,v)| json!([k,format!("{v:?}")])).collect::<Vec<_>>()).collect::<Vec<_>>(),
        "metadata":{"backend":m.backend,"operation":format!("{:?}",m.operation),"result_count":m.result_count,
            "statements":m.statements.len(),"sql_log_default":m.sql_log==Default::default(),"debug_query":m.debug_query,
            "affected_rows":m.affected_rows,"trace":m.trace_chain.len(),"comment":m.comment,
            "backend_request_id":m.backend_request_id,"parameterized_query":m.parameterized_query,"params":m.params.len(),
            "timestamps_ordered":m.started_at<=m.ended_at}})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
