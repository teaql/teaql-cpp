use serde_json::{Value as Json,json};
use std::{collections::HashMap,io::{self,BufRead},time::SystemTime};
use teaql_core::*;
use teaql_core::request::FacetRequest;
use teaql_core::dynamic_fields::{DynamicFieldDefinitions,DynamicFieldValues,DynamicFieldSelection};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;
use teaql_runtime::{ExprEvaluator,InMemoryQueryEngine};
fn formatted<T:std::fmt::Debug>(v:&T)->Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),"lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),"pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),"width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),"zero_pad":format!("{v:+012?}")})
}
fn pattern(input:&Json)->Json {
    let p=Expr::LikePattern {pattern:input["pattern"].as_str().unwrap().to_owned(),original:input["original"].as_str().unwrap().to_owned()};
    let row=if input["text"].is_null(){CompactRow::new(vec![].into(),vec![])}else{CompactRow::new(vec!["x".to_owned()].into(),vec![Value::Text(input["text"].as_str().unwrap().to_owned())])};
    let expr=match input["wrapper"].as_u64().unwrap() {
        0=>p,1=>Expr::IsNull(Box::new(p)),2=>Expr::IsNotNull(Box::new(p)),3=>Expr::Not(Box::new(p)),
        _=>Expr::Binary {left:Box::new(Expr::column("x")),op:[BinaryOp::Eq,BinaryOp::Ne,BinaryOp::Gt,BinaryOp::Gte,BinaryOp::Lt,BinaryOp::Lte,BinaryOp::Like,BinaryOp::NotLike,BinaryOp::In,BinaryOp::NotIn,BinaryOp::InLarge,BinaryOp::NotInLarge][input["operator"].as_u64().unwrap() as usize],right:Box::new(p)},
    };
    json!({"eval":ExprEvaluator::eval(&expr,&row),"resolved":formatted(&ExprEvaluator::resolve(&expr,&row))})
}
fn row(profile:u64,index:u64)->CompactRow {
    let mut row=CompactRow::new(vec!["id".into(),"group".into(),"label".into(),"_rank".into()].into(),vec![Value::U64(index),Value::U64(index%2),Value::Text(format!("label{index}\0雪")),Value::I64(index as i64)]);
    if [1,2,8].contains(&profile) {
        let list=if profile==8{SmartList::new(vec![CompactRow::new(vec!["child".into()].into(),vec![Value::Text("child\0雪".repeat(32))])])}else{SmartList::new(vec![])};
        row.set_loaded_relation("lines".into(),list);if profile==2 {row.take_loaded_relation("lines");}
        row.mark_relation_loaded("lines",&mut RelationShapeCache::default());
    }
    if [3,4,5,8,9].contains(&profile) {
        let definitions=DynamicFieldDefinitions::new("Item","v1",[("memo".into(),DataType::Text)]).unwrap();
        let mut fields=HashMap::new();
        if profile==4 {fields.insert("memo".into(),Value::Null);}
        if [5,8].contains(&profile){fields.insert("memo".into(),Value::Text("dynamic\0雪".repeat(32)));}
        row.set_loaded_dynamic_fields(DynamicFieldValues::from_values(definitions,fields).unwrap());
        if profile==9 {row.take_loaded_dynamic_fields();}
    }
    if [5,6,7,8,9].contains(&profile) {
        let fields=if [6,9].contains(&profile){vec![]}else{vec![("_rank".to_owned(),DataType::I64)]};
        CompactRow::attach_dynamic_property_definitions(std::slice::from_mut(&mut row),DynamicPropertyDefinitions::new(fields).unwrap());
    }
    row
}
fn query(mode:u64,rich:bool)->SelectQuery {
    let mut q=SelectQuery::new("Item");
    if rich {
        let ignored=SelectQuery::new("Ignored").limit(20001);
        q.hard_limit=1;q.expr_projection.push(NamedExpr::new("computed",Expr::value(7u64)));
        q.search_with_text=Some("ignored".into());q.having=Some(Expr::value(false));
        q.partition_by=Some("ignored".into());q.top_n_probe_parent_threshold=Some(0);
        q.group_by.push("ignored".into());q.relations.push(RelationLoad::with_query("lines",ignored.clone()));
        q.facets.push(FacetRequest::new("facet","lines",ignored.clone(),true));
        q.relation_aggregates.push(RelationAggregate::new("lines","related",ignored.clone(),false));
        q.aggregation_cache=Some(AggregationCacheOptions::enabled(7).propagate(8));
        q=q.comment("ignored comment").purpose("ignored purpose").raw_sql("not SQL").raw_sql_search_criteria("ignored").dynamic_property_raw("_request","7")
            .with_dynamic_property_definitions(DynamicPropertyDefinitions::new([("_request".into(),DataType::I64)]).unwrap())
            .select_dynamic_fields(DynamicFieldSelection::All).project_raw("raw","8")
            .object_group_by("object","owner",ignored.clone()).child_enhancement(ignored)
            .stream(0).optimize_for_continuous_page_fetch().optimize_pagination_with_id_set();
    }
    match mode {
        1=>{q=q.order_desc("id").page(1,1);},
        2=>{q=q.project("id");},
        3=>{q.group_by.clear();q=q.count("count").page(999,0).project("absent");},
        4=>{q.group_by=vec!["group".into()];q=q.count("count").page(999,0).project("absent");},_=>{},
    }
    q
}
fn execute(input:&Json)->Json {
    let q=query(input["mode"].as_u64().unwrap(),input["rich"].as_bool().unwrap());
    let rows:Vec<_>=(0..input["count"].as_u64().unwrap()).map(|i|row(input["profile"].as_u64().unwrap(),i)).collect();
    let before=rows.iter().map(formatted).collect::<Vec<_>>();let borrowed=input["borrowed"].as_bool().unwrap();
    let start=SystemTime::now();
    let result=InMemoryQueryEngine::execute(&q,rows.clone());let end=SystemTime::now();
    let m=&result.metadata;
    json!({"rows":result.rows.iter().map(formatted).collect::<Vec<_>>(),
        "input_unchanged":if borrowed{Some(before==rows.iter().map(formatted).collect::<Vec<_>>())}else{None},
        "metadata":{"backend":m.backend,"operation":format!("{:?}",m.operation),"result_count":m.result_count,
        "statements":m.statements.len(),"sql_log_default":m.sql_log==Default::default(),"debug_query":m.debug_query,
        "affected_rows":m.affected_rows,"trace":m.trace_chain.len(),"comment":m.comment,"backend_request_id":m.backend_request_id,
        "parameterized_query":m.parameterized_query,"params":m.params.len(),
        "started_within_call":start<=m.started_at && m.started_at<=end,"ended_within_call":start<=m.ended_at && m.ended_at<=end,"ordered":m.started_at<=m.ended_at}})
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",if input["kind"]=="pattern"{pattern(&input)}else{execute(&input)});}}
