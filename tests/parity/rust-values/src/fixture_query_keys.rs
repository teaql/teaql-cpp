use teaql_core::*;
use serde_json::Value as J;
use crate::{fixture_sql,fixture_value::value};
fn items<'a>(j:&'a J,key:&str)->&'a [J]{j[key].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn op(i:u64)->BinaryOp{[BinaryOp::Eq,BinaryOp::Ne,BinaryOp::Gt,BinaryOp::Gte,BinaryOp::Lt,BinaryOp::Lte,BinaryOp::Like,BinaryOp::NotLike,BinaryOp::In,BinaryOp::NotIn,BinaryOp::InLarge,BinaryOp::NotInLarge][i as usize]}
fn entity(j:&J)->EntityDescriptor{
    let mut e=fixture_sql::entity(j);e.data_service=j["service"].as_str().map(str::to_owned);e.audit_value_max_len=j["audit_max"].as_u64().map(|v|v as usize);
    for p in items(j,"relations"){let mut rel=RelationDescriptor::new(p["name"].as_str().unwrap(),p["target"].as_str().unwrap());if let Some(v)=p["local"].as_str(){rel.local_key=v.into();}if let Some(v)=p["foreign"].as_str(){rel.foreign_key=v.into();}rel.many=p["many"].as_bool().unwrap_or(false);rel.attach=p["attach"].as_bool().unwrap_or(true);rel.delete_missing=p["delete_missing"].as_bool().unwrap_or(true);e.relations.push(rel);}e
}
pub fn expression(j:&J)->Expr{
    let args=||items(j,"args").iter().map(expression).collect::<Vec<_>>();
    match j["kind"].as_str().unwrap(){
        "column"=>Expr::column(j["name"].as_str().unwrap()),"value"=>Expr::Value(value(&j["value"])),
        "binary"=>Expr::Binary{left:Box::new(expression(&j["left"])),op:op(j["op"].as_u64().unwrap()),right:Box::new(expression(&j["right"]))},
        "function"=>Expr::Function{function:[ExprFunction::Soundex,ExprFunction::Gbk,ExprFunction::Count,ExprFunction::Sum,ExprFunction::Avg,ExprFunction::Min,ExprFunction::Max,ExprFunction::Stddev,ExprFunction::StddevPop,ExprFunction::VarSamp,ExprFunction::VarPop,ExprFunction::BitAnd,ExprFunction::BitOr,ExprFunction::BitXor][j["function"].as_u64().unwrap() as usize],args:args()},
        "subquery"=>Expr::SubQuery{left:Box::new(expression(&j["left"])),op:op(j["op"].as_u64().unwrap()),entity:entity(&j["entity"]),query:Box::new(query(&j["query"]))},
        "between"=>Expr::Between{expr:Box::new(expression(&j["args"][0])),lower:Box::new(expression(&j["args"][1])),upper:Box::new(expression(&j["args"][2]))},
        "null"=>Expr::IsNull(Box::new(expression(&j["args"][0]))),"not_null"=>Expr::IsNotNull(Box::new(expression(&j["args"][0]))),"not"=>Expr::Not(Box::new(expression(&j["args"][0]))),
        "and"=>Expr::And(args()),"or"=>Expr::Or(args()),_=>panic!("unknown expression")
    }
}
pub fn query(j:&J)->SelectQuery{
    let mut q=fixture_sql::query(j);
    if let Some(v)=j["entity"].as_str(){q.entity=v.into();}if let Some(v)=j["hard_limit"].as_u64(){q.hard_limit=v;}
    q.top_n_probe_parent_threshold=j["threshold"].as_u64().map(|v|v as usize);
    q.filter=j.get("filter").map(expression);q.having=j.get("having").map(expression);
    q.expr_projection=items(j,"expr_projection").iter().map(|p|NamedExpr::new(p["alias"].as_str().unwrap(),expression(&p["expr"]))).collect();
    q.order_by=items(j,"order").iter().map(|o|OrderBy{field:o["field"].as_str().unwrap_or("").into(),expr:o.get("expr").map(expression),direction:if o["direction"]==0{SortDirection::Asc}else{SortDirection::Desc}}).collect();
    q.relations=items(j,"relations").iter().map(|l|RelationLoad{name:l["name"].as_str().unwrap().into(),query:l.get("query").map(|q|Box::new(query(q)))}).collect();
    q.object_group_bys=items(j,"object_groups").iter().map(|g|ObjectGroupBy::new(g["property"].as_str().unwrap(),g["storage"].as_str().unwrap(),query(&g["query"]))).collect();
    q.child_enhancements=items(j,"children").iter().map(query).collect();
    q.aggregation_cache=j.get("cache").map(|v|AggregationCacheOptions{enabled:v["enabled"].as_bool().unwrap(),cache_expired_millis:v["age"].as_u64().unwrap(),propagate:v["propagate"].as_bool().unwrap(),propagate_cache_expired_millis:v["propagate_age"].as_u64().unwrap()});
    q.stream_config=j["stream"].as_u64().map(|v|StreamConfig{chunk_size:v as usize});
    q.continuous_page_fetch=j.get("continuous").map(|v|ContinuousPageFetchOptions{namespace:v["namespace"].as_str().unwrap().into(),ttl_seconds:v["ttl"].as_u64().unwrap()});
    q.id_set_pagination=j.get("ids").map(|v|IdSetPaginationOptions{namespace:v["namespace"].as_str().unwrap().into(),ttl_seconds:v["ttl"].as_u64().unwrap(),max_ids:v["max"].as_u64().unwrap()});
    q.trace_chain=items(j,"trace").iter().map(|n|TraceNode{kind:[TraceKind::Operation,TraceKind::Request,TraceKind::Relation,TraceKind::Entity,TraceKind::Provider,TraceKind::Sql,TraceKind::Comment,TraceKind::Purpose,TraceKind::AuditReason][n["kind"].as_u64().unwrap() as usize],entity_type:n["entity"].as_str().unwrap().into(),entity_id:n["id"].as_u64(),comment:n["comment"].as_str().unwrap().into()}).collect();q
}
