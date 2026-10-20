//! Assemble public original models; algorithms remain in the locked library.
use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, sync::Arc};
use teaql_core::*;
use teaql_core::request::FacetRequest;
use teaql_core::dynamic_fields::DynamicFieldSelection;
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn schema(changed: bool) -> Arc<DynamicPropertyDefinitions> {
    DynamicPropertyDefinitions::new([(if changed {"_changed"} else {"_rank"}.into(),
        if changed {DataType::Text} else {DataType::U64})]).unwrap()
}

fn selection(changed: bool) -> DynamicFieldSelection {
    DynamicFieldSelection::fields([(if changed {"changed"} else {"note"}.into(),
        if changed {DataType::Text} else {DataType::I64})]).unwrap()
}

fn make(profile: usize) -> SelectQuery {
    if profile == 0 { return SelectQuery::new("Order"); }
    let n = [1, 7, 10000, u64::MAX][profile];
    let text = match profile { 2 => "state\0雪".to_owned(), 3 => "x".repeat(128), _ => "state".to_owned() };
    let ignored = SelectQuery::new("Ignored").limit(20001).hard_limit(1);
    let child = SelectQuery::new("Child").limit(10000).hard_limit(1);
    let enhancement = SelectQuery::new("Enhancement").hard_limit(1);
    let expression = Expr::in_subquery("id", EntityDescriptor::new("Ignored"), ignored.clone(), "id");
    let mut query = SelectQuery::new(text.clone()).hard_limit(n).project("id")
        .project_expr("computed", Expr::value(7u64)).search_with_text(text.clone())
        .filter(expression).having(Expr::is_not_null("_rank"))
        .order_expr_desc(Expr::column("id"))
        .page(n, if profile == 2 {0} else if profile == 3 {u64::MAX} else {3})
        .partition_by(text.clone()).top_n_probe_parent_threshold(if profile == 2 {0} else {13})
        .sum("amount", "total").group_by("owner").relation("plain").relation_query("child", child)
        .enable_aggregation_cache_for(n).propagate_aggregation_cache(2)
        .comment(text.clone()).purpose(text.clone()).raw_sql(text.clone())
        .raw_sql_search_criteria(text.clone()).dynamic_property_raw("_rank", "1")
        .with_dynamic_property_definitions(schema(false)).select_dynamic_fields(selection(false))
        .project_raw("raw", "42").object_group_by("object", "owner", ignored.clone())
        .child_enhancement(enhancement).stream(if profile == 2 {0} else {1000})
        .optimize_for_continuous_page_fetch_with(text.clone(), n)
        .optimize_pagination_with_id_set_config(text.clone(), n, n);
    query.facets.push(FacetRequest::new("facet", "children", ignored.clone(), true));
    query.relation_aggregates.push(RelationAggregate::new("children", "related", ignored, false));
    query.trace_chain.push(TraceNode {kind:TraceKind::Operation,entity_type:text.clone(),
                                    entity_id:Some(0),comment:text});
    query
}

fn mutate(query: &mut SelectQuery, field: usize) {
    let child = SelectQuery::new("Changed").limit(30000);
    let changed = Expr::eq("changed", 42u64);
    match field {
        0 => query.hard_limit = 12345,
        1 => query.entity = "Changed".into(),
        2 => query.projection.push("changed".into()),
        3 => query.expr_projection.push(NamedExpr::new("changed", changed)),
        4 => query.search_with_text = Some("changed".into()),
        5 => query.filter = Some(changed),
        6 => query.having = Some(changed),
        7 => query.order_by.push(OrderBy::asc("changed")),
        8 => query.slice = Some(Slice {limit:Some(0),offset:u64::MAX}),
        9 => query.partition_by = Some("changed".into()),
        10 => query.top_n_probe_parent_threshold = Some(42),
        11 => query.aggregates.push(Aggregate::count("changed")),
        12 => query.group_by.push("changed".into()),
        13 => query.relations.push(RelationLoad::new("changed")),
        14 => query.facets.push(FacetRequest::new("changed", "changed", child, false)),
        15 => query.relation_aggregates.push(RelationAggregate::new("changed", "changed", child, true)),
        16 => query.aggregation_cache = Some(AggregationCacheOptions {enabled:false,cache_expired_millis:42,propagate:true,propagate_cache_expired_millis:43}),
        17 => query.comment = Some("changed".into()),
        18 => query.purpose = Some("changed".into()),
        19 => query.trace_chain.push(TraceNode {kind:TraceKind::Purpose,entity_type:"changed".into(),entity_id:None,comment:"changed".into()}),
        20 => query.raw_sql = Some("changed".into()),
        21 => query.raw_sql_search_criteria.push("changed".into()),
        22 => query.dynamic_properties.push(RawSqlProjection::new("_changed", "43")),
        23 => query.dynamic_property_definitions = Some(schema(true)),
        24 => query.dynamic_field_selection = Some(Arc::new(selection(true))),
        25 => query.raw_projections.push(RawSqlProjection::new("changed", "43")),
        26 => query.object_group_bys.push(ObjectGroupBy::new("changed", "changed", child)),
        27 => query.child_enhancements.push(child),
        28 => query.stream_config = Some(StreamConfig {chunk_size:42}),
        29 => query.continuous_page_fetch = Some(ContinuousPageFetchOptions::new("changed", 42)),
        30 => query.id_set_pagination = Some(IdSetPaginationOptions::new("changed", 42, 43)),
        _ => panic!("Unknown query field"),
    }
}

fn prepared(query: &SelectQuery) -> Json {
    match query.clone().prepare_for_list() {
        Ok(value) => json!({"ok":formatted(&value)}),
        Err(error) => json!({"error":error}),
    }
}

fn observe(input: &Json) -> Json {
    let base = make(input["profile"].as_u64().unwrap() as usize);
    let original = base.clone();
    let mut changed = base.clone();
    mutate(&mut changed, input["field"].as_u64().unwrap() as usize);
    let copy = changed.clone();
    json!({"base":formatted(&base),"changed":formatted(&changed),
        "clone":formatted(&copy),"self_equal":changed==changed,"equal_clone":changed==copy,
        "changed_equal_base":changed==base,"base_unchanged":base==original,
        "schema_shared":changed.dynamic_property_definitions.as_ref().map(|a|
            Arc::ptr_eq(a, copy.dynamic_property_definitions.as_ref().unwrap())),
        "selection_shared":changed.dynamic_field_selection.as_ref().map(|a|
            Arc::ptr_eq(a, copy.dynamic_field_selection.as_ref().unwrap())),
        "base_prepared":prepared(&base),"changed_prepared":prepared(&changed)})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
