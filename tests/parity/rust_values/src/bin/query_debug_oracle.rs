use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::*;
#[path = "../fixture_value.rs"]
mod fixture_value;

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}

fn evaluate(input: Json) -> Json {
    let flags = input["flags"].as_u64().unwrap();
    let n: u64 = input["number"].as_str().unwrap().parse().unwrap();
    let payload = fixture_value::value(&input["value"]);
    let binary_ops = [BinaryOp::Eq,BinaryOp::Ne,BinaryOp::Gt,BinaryOp::Gte,BinaryOp::Lt,BinaryOp::Lte,
        BinaryOp::Like,BinaryOp::NotLike,BinaryOp::In,BinaryOp::NotIn,BinaryOp::InLarge,BinaryOp::NotInLarge];
    let functions = [ExprFunction::Soundex,ExprFunction::Gbk,ExprFunction::Count,ExprFunction::Sum,
        ExprFunction::Avg,ExprFunction::Min,ExprFunction::Max,ExprFunction::Stddev,ExprFunction::StddevPop,
        ExprFunction::VarSamp,ExprFunction::VarPop,ExprFunction::BitAnd,ExprFunction::BitOr,ExprFunction::BitXor];
    let aggregates = [AggregateFunction::Count,AggregateFunction::Sum,AggregateFunction::Avg,
        AggregateFunction::Min,AggregateFunction::Max,AggregateFunction::Stddev,AggregateFunction::StddevPop,
        AggregateFunction::VarSamp,AggregateFunction::VarPop,AggregateFunction::BitAnd,AggregateFunction::BitOr,AggregateFunction::BitXor];
    let trace_kinds = [TraceKind::Operation,TraceKind::Request,TraceKind::Relation,TraceKind::Entity,
        TraceKind::Provider,TraceKind::Sql,TraceKind::Comment,TraceKind::Purpose,TraceKind::AuditReason];
    let op = binary_ops[input["op"].as_u64().unwrap() as usize];
    let function = functions[input["function"].as_u64().unwrap() as usize];
    let aggregate_function = aggregates[input["aggregate"].as_u64().unwrap() as usize];
    let direction = if flags & 1 != 0 { SortDirection::Desc } else { SortDirection::Asc };
    let column = Expr::column("field\n雪");
    let literal = Expr::value(payload);
    let binary = Expr::Binary {left:Box::new(column.clone()),op,right:Box::new(literal.clone())};
    let mut child = SelectQuery::new("Child");
    child.projection = vec!["id".into(),"value".into()];
    child.filter = Some(binary.clone());
    child.slice = Some(Slice {limit:Some(2),offset:1});
    let entity = EntityDescriptor::new("Child")
        .property(PropertyDescriptor::new("id",DataType::U64).id())
        .property(PropertyDescriptor::new("secret",DataType::Text).max_length(0))
        .relation(RelationDescriptor::new("owner","Owner").detached())
        .audit_mask_fields(vec!["secret".into()]).audit_value_max_len(Some(0));
    let expressions = vec![column.clone(), literal.clone(),
        Expr::function(function,[column.clone(),literal.clone()]), binary.clone(),
        Expr::SubQuery {left:Box::new(column.clone()),op,entity,query:Box::new(child.clone())},
        Expr::Between {expr:Box::new(column.clone()),lower:Box::new(literal.clone()),upper:Box::new(Expr::value(9i64))},
        Expr::IsNull(Box::new(column.clone())),Expr::IsNotNull(Box::new(column.clone())),
        Expr::And(vec![binary.clone(),literal.clone()]),Expr::Or(Vec::new()),Expr::Not(Box::new(binary.clone()))];
    let named = NamedExpr::new("alias\0",binary.clone());
    let order = if flags & 1 != 0 { OrderBy::expr(binary.clone(),direction) } else { OrderBy::new("field",direction) };
    let aggregate = Aggregate::new(aggregate_function,"amount","total");
    let slice = Slice {limit:if flags & 2 != 0 {Some(n)} else {None},offset:n};
    let relation = if flags & 2 != 0 {RelationLoad::with_query("children",child.clone())} else {RelationLoad::new("children")};
    let relation_aggregate = RelationAggregate::new("children","count",child.clone(),flags & 1 != 0);
    let raw = RawSqlProjection::new("derived","sensitive raw SQL\n");
    let group = ObjectGroupBy::new("group","group_id",child.clone());
    let cache = AggregationCacheOptions {enabled:flags & 1 != 0,cache_expired_millis:n,
        propagate:flags & 2 != 0,propagate_cache_expired_millis:n};
    let stream = StreamConfig {chunk_size:if flags & 1 != 0 {0} else {1000}};
    let continuous = ContinuousPageFetchOptions::new("namespace",n.max(1));
    let id_set = IdSetPaginationOptions::new("namespace",n.max(1),n.max(1));
    let trace = TraceNode::typed(trace_kinds[input["trace"].as_u64().unwrap() as usize],"Order",
        if flags & 2 != 0 {Some(n)} else {None},"private comment\n雪");
    let mut query = SelectQuery::new("Order\n雪");
    if flags & 4 != 0 {
        query.hard_limit=n; query.projection=vec!["id".into(),"id".into(),"".into()];
        query.expr_projection=vec![named.clone()];query.search_with_text=Some(String::new());
        query.filter=Some(expressions[4].clone());query.having=Some(expressions[5].clone());
        query.order_by=vec![order.clone()];query.slice=Some(slice);query.partition_by=Some("partition".into());
        query.top_n_probe_parent_threshold=Some(0);query.aggregates=vec![aggregate.clone()];
        query.group_by=vec!["status".into()];query.relations=vec![relation.clone()];
        query.aggregation_cache=Some(cache);query.comment=Some(String::new());query.trace_chain=vec![trace.clone()];
        query.raw_sql=Some(String::new());query.raw_sql_search_criteria=vec!["raw = 1".into()];
        query.dynamic_properties=vec![raw.clone()];query.raw_projections=vec![raw.clone()];
        query.object_group_bys=vec![group.clone()];query.child_enhancements=vec![child];
        query.stream_config=Some(stream.clone());query.continuous_page_fetch=Some(continuous.clone());
        query.id_set_pagination=Some(id_set.clone());
    }
    let mut output=json!({"expressions":expressions.iter().map(formatted).collect::<Vec<_>>(),
        "query":formatted(&query),"named":formatted(&named),"order":formatted(&order),
        "aggregate":formatted(&aggregate),"slice":formatted(&slice),"relation":formatted(&relation),
        "relation_aggregate":formatted(&relation_aggregate),"raw":formatted(&raw),"group":formatted(&group),
        "cache":formatted(&cache),"stream":formatted(&stream),"continuous":formatted(&continuous),
        "id_set":formatted(&id_set),"trace":formatted(&trace)});
    if input["enums"] == true {
        output["enums"]=json!({"binary":binary_ops.iter().map(formatted).collect::<Vec<_>>(),
            "functions":functions.iter().map(formatted).collect::<Vec<_>>(),
            "aggregate":aggregates.iter().map(formatted).collect::<Vec<_>>(),
            "direction":[formatted(&SortDirection::Asc),formatted(&SortDirection::Desc)],
            "trace":trace_kinds.iter().map(formatted).collect::<Vec<_>>()});
    }
    output
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
