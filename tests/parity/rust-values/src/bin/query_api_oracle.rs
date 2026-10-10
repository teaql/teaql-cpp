//! Observe unchanged core public constructors/builders, without SQL compilation.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{AggregationCacheOptions, BinaryOp, EntityDescriptor, Expr, ExprFunction, SelectQuery, OrderBy, SortDirection, Aggregate, AggregateFunction, NamedExpr, RelationLoad, RelationAggregate, RawSqlProjection, ObjectGroupBy, ContinuousPageFetchOptions, IdSetPaginationOptions, StreamConfig, Slice};
#[path = "../fixture_value.rs"]
mod fixture_value;

fn full_expression(input: &Json) -> Json {
    let column = input["column"].as_str().unwrap();
    let pattern = input["pattern"].as_str().unwrap();
    let lower = fixture_value::value(&input["lower"]);
    let upper = fixture_value::value(&input["upper"]);
    let base = Expr::value(lower.clone());
    let mut out = serde_json::Map::new();
    let mut record = |label: &str, expr: Expr| {
        out.insert(label.into(), json!({"debug":format!("{expr:?}"),
            "self":expr == expr, "clone":expr == expr.clone()}));
    };
    record("column", Expr::column(column));
    record("value", Expr::value(lower.clone()));
    record("eq", Expr::eq(column, lower.clone()));
    record("ne", Expr::ne(column, lower.clone()));
    record("gt", Expr::gt(column, lower.clone()));
    record("gte", Expr::gte(column, lower.clone()));
    record("lt", Expr::lt(column, lower.clone()));
    record("lte", Expr::lte(column, lower.clone()));
    record("sound_like", Expr::sound_like(column, lower.clone()));
    record("between", Expr::between(column, lower, upper));
    record("between_scalar", Expr::between(column, i64::MIN, u64::MAX));
    record("between_text", Expr::between(column, pattern, ""));
    record("like", Expr::like(column, pattern));
    record("not_like", Expr::not_like(column, pattern));
    record("contain", Expr::contain(column, pattern));
    record("not_contain", Expr::not_contain(column, pattern));
    record("begin_with", Expr::begin_with(column, pattern));
    record("not_begin_with", Expr::not_begin_with(column, pattern));
    record("end_with", Expr::end_with(column, pattern));
    record("not_end_with", Expr::not_end_with(column, pattern));
    record("is_null", Expr::is_null(column));
    record("is_not_null", Expr::is_not_null(column));
    record("count_all", Expr::count_all());
    let unary: [(&str, fn(Expr) -> Expr); 14] = [
        ("soundex", Expr::soundex), ("gbk", Expr::gbk), ("count_expr", Expr::count_expr),
        ("sum_expr", Expr::sum_expr), ("avg_expr", Expr::avg_expr), ("min_expr", Expr::min_expr),
        ("max_expr", Expr::max_expr), ("stddev_expr", Expr::stddev_expr),
        ("stddev_pop_expr", Expr::stddev_pop_expr), ("var_samp_expr", Expr::var_samp_expr),
        ("var_pop_expr", Expr::var_pop_expr), ("bit_and_expr", Expr::bit_and_expr),
        ("bit_or_expr", Expr::bit_or_expr), ("bit_xor_expr", Expr::bit_xor_expr),
    ];
    for (label, function) in unary { record(label, function(base.clone())); }
    record("negate", Expr::negate(base.clone()));
    let nested = Expr::and([base.clone(), Expr::is_null(column), base.clone()]);
    record("and_expr_equal", base.clone().and_expr(base.clone()));
    record("or_expr_equal", base.clone().or_expr(base.clone()));
    record("and_expr_nested_right", base.clone().and_expr(nested.clone()));
    record("and_expr_existing", nested.clone().and_expr(base.clone()));
    record("or_expr_nested_right", base.clone().or_expr(nested.clone()));
    record("or_expr_existing", Expr::or([base.clone(), Expr::is_null(column)]).or_expr(base.clone()));
    let operations = [BinaryOp::Eq, BinaryOp::Ne, BinaryOp::Gt, BinaryOp::Gte,
        BinaryOp::Lt, BinaryOp::Lte, BinaryOp::Like, BinaryOp::NotLike,
        BinaryOp::In, BinaryOp::NotIn, BinaryOp::InLarge, BinaryOp::NotInLarge];
    let entity = EntityDescriptor::new("Child");
    let mut query = SelectQuery::new("Child").project("obsolete").comment(pattern);
    if input["filter"] == true { query = query.filter(Expr::eq("name", fixture_value::value(&input["lower"]))); }
    for (index, op) in operations.into_iter().enumerate() {
        record(&format!("binary_{index}"), Expr::binary(base.clone(), op, Expr::column(column)));
        record(&format!("columns_{index}"), Expr::compare_columns(column, op, "other"));
        record(&format!("subquery_{index}"), Expr::subquery(base.clone(), op, entity.clone(), query.clone(), column));
    }
    record("in_subquery", Expr::in_subquery("id", entity.clone(), query.clone(), column));
    record("not_in_subquery", Expr::not_in_subquery("id", entity, query, column));
    out.insert("threshold".into(), json!(Expr::LARGE_IN_THRESHOLD));
    Json::Object(out)
}

fn query_state(query: &SelectQuery) -> Json {
    json!({"debug":format!("{query:?}"),"self":query == query,"clone":query == &query.clone()})
}
fn query_builders(input: &Json) -> Json {
    let field = input["column"].as_str().unwrap();
    let pattern = input["pattern"].as_str().unwrap();
    let base = Expr::eq(field, fixture_value::value(&input["value"]));
    let child = SelectQuery::new("Child").filter(base.clone()).page(3,5).hard_limit(2);
    let ignored = SelectQuery::new("Ignored").limit(20_000);
    let mut query = SelectQuery::new("Order雪\0");
    let mut states = vec![query_state(&query)];
    query = query.project(field);
    states.push(query_state(&query));
    query = query.project_expr("expression", base.clone());
    states.push(query_state(&query));
    query = query.project_raw("raw", "coalesce(value, 0)");
    states.push(query_state(&query));
    query = query.dynamic_property_raw("dynamic", pattern);
    states.push(query_state(&query));
    query = query.search_with_text("");
    states.push(query_state(&query));
    query = query.search_with_text(pattern);
    states.push(query_state(&query));
    query = query.filter(base.clone());
    states.push(query_state(&query));
    query = query.and_filter(base.clone());
    states.push(query_state(&query));
    query = query.or_filter(Expr::is_null(field));
    states.push(query_state(&query));
    query = query.having(base.clone());
    states.push(query_state(&query));
    query = query.and_having(base.clone());
    states.push(query_state(&query));
    query = query.or_having(Expr::is_null(field));
    states.push(query_state(&query));
    query = query.order_by(OrderBy::new(field, SortDirection::Desc));
    states.push(query_state(&query));
    query = query.order_asc(field);
    states.push(query_state(&query));
    query = query.order_desc(field);
    states.push(query_state(&query));
    query = query.order_expr_asc(base.clone());
    states.push(query_state(&query));
    query = query.order_expr_desc(base.clone());
    states.push(query_state(&query));
    query = query.order_gbk_asc(field);
    states.push(query_state(&query));
    query = query.order_gbk_desc(field);
    states.push(query_state(&query));
    query = query.group_by(field);
    states.push(query_state(&query));
    query = query.aggregate(Aggregate::new(AggregateFunction::Sum, field, "sum"));
    states.push(query_state(&query));
    query = query.count("count");
    states.push(query_state(&query));
    query = query.count_field(field, "count_field");
    states.push(query_state(&query));
    query = query.sum(field, "sum");
    states.push(query_state(&query));
    query = query.avg(field, "avg");
    states.push(query_state(&query));
    query = query.min(field, "min");
    states.push(query_state(&query));
    query = query.max(field, "max");
    states.push(query_state(&query));
    query = query.stddev(field, "stddev");
    states.push(query_state(&query));
    query = query.stddev_pop(field, "stddev_pop");
    states.push(query_state(&query));
    query = query.var_samp(field, "var_samp");
    states.push(query_state(&query));
    query = query.var_pop(field, "var_pop");
    states.push(query_state(&query));
    query = query.bit_and(field, "bit_and");
    states.push(query_state(&query));
    query = query.bit_or(field, "bit_or");
    states.push(query_state(&query));
    query = query.bit_xor(field, "bit_xor");
    states.push(query_state(&query));
    query = query.propagate_aggregation_cache(u64::MAX);
    states.push(query_state(&query));
    query = query.enable_aggregation_cache();
    states.push(query_state(&query));
    query = query.enable_aggregation_cache_for(7);
    states.push(query_state(&query));
    query = query.comment("");
    states.push(query_state(&query));
    query = query.comment(pattern);
    states.push(query_state(&query));
    query = query.raw_sql(pattern);
    states.push(query_state(&query));
    query = query.raw_sql_search_criteria(pattern);
    states.push(query_state(&query));
    query = query.object_group_by("object", field, ignored.clone());
    states.push(query_state(&query));
    query = query.child_enhancement(child.clone());
    states.push(query_state(&query));
    query = query.relation("absent");
    states.push(query_state(&query));
    query = query.relation_query("configured", child.clone());
    states.push(query_state(&query));
    query = query.offset(u64::MAX);
    states.push(query_state(&query));
    query = query.limit(0);
    states.push(query_state(&query));
    query = query.page(9, 7);
    states.push(query_state(&query));
    query = query.hard_limit(20);
    states.push(query_state(&query));
    query = query.optimize_for_continuous_page_fetch();
    states.push(query_state(&query));
    query = query.optimize_for_continuous_page_fetch_with(" ns雪 ", u64::MAX);
    states.push(query_state(&query));
    query = query.optimize_pagination_with_id_set();
    states.push(query_state(&query));
    query = query.optimize_pagination_with_id_set_config(" ns雪 ", u64::MAX, u64::MAX);
    states.push(query_state(&query));
    query = query.partition_by(field);
    states.push(query_state(&query));
    query = query.top_n_probe_parent_threshold(0);
    states.push(query_state(&query));
    query = query.stream(0);
    states.push(query_state(&query));
    query = query.stream_default();
    states.push(query_state(&query));
    let prepared = query.clone().prepare_for_list().map(|q| query_state(&q));
    let too_large = query.clone().limit(21).prepare_for_list().unwrap_err();
    let mut nested_error = query.clone();
    nested_error.relations[1].query.as_mut().unwrap().slice.as_mut().unwrap().limit = Some(10_001);
    let nested_error = nested_error.prepare_for_list().unwrap_err();
    json!({"states":states,"prepared":prepared.unwrap(),"outer_error":too_large,"nested_error":nested_error})
}

fn helper_state<T: std::fmt::Debug + Clone + PartialEq>(value: T) -> Json {
    json!({"debug":format!("{value:?}"),"self":value == value,"clone":value == value.clone()})
}
fn query_helpers(input: &Json) -> Json {
    let field = input["column"].as_str().unwrap();
    let pattern = input["pattern"].as_str().unwrap();
    let base = Expr::eq(field, fixture_value::value(&input["value"]));
    let child = SelectQuery::new("Child").filter(base.clone());
    let mut out = serde_json::Map::new();
    out.insert("named".into(), helper_state(NamedExpr::new(pattern, base.clone())));
    out.insert("order_new".into(), helper_state(OrderBy::new(field, SortDirection::Desc)));
    out.insert("order_expr".into(), helper_state(OrderBy::expr(base.clone(), SortDirection::Asc)));
    out.insert("order_asc".into(), helper_state(OrderBy::asc(field)));
    out.insert("order_desc".into(), helper_state(OrderBy::desc(field)));
    out.insert("order_asc_expr".into(), helper_state(OrderBy::asc_expr(base.clone())));
    out.insert("order_desc_expr".into(), helper_state(OrderBy::desc_expr(base.clone())));
    out.insert("order_asc_gbk".into(), helper_state(OrderBy::asc_gbk(field)));
    out.insert("order_desc_gbk".into(), helper_state(OrderBy::desc_gbk(field)));
    out.insert("aggregate_new".into(), helper_state(Aggregate::new(AggregateFunction::BitXor, field, pattern)));
    out.insert("aggregate_count".into(), helper_state(Aggregate::count(pattern)));
    out.insert("relation_new".into(), helper_state(RelationLoad::new(field)));
    out.insert("relation_query".into(), helper_state(RelationLoad::with_query(field, child.clone())));
    out.insert("relation_aggregate".into(), helper_state(RelationAggregate::new(field, pattern, child.clone(), true)));
    out.insert("raw".into(), helper_state(RawSqlProjection::new(field, pattern)));
    out.insert("object_group".into(), helper_state(ObjectGroupBy::new(field, pattern, child)));
    out.insert("cache_enabled".into(), helper_state(AggregationCacheOptions::enabled(u64::MAX)));
    out.insert("cache_propagated".into(), helper_state(AggregationCacheOptions::enabled(7).propagate(u64::MAX)));
    out.insert("stream_default".into(), helper_state(StreamConfig::default()));
    out.insert("stream_zero".into(), helper_state(StreamConfig { chunk_size: 0 }));
    out.insert("continuous".into(), helper_state(ContinuousPageFetchOptions::new(" ns雪 ", u64::MAX)));
    out.insert("id_set".into(), helper_state(IdSetPaginationOptions::new(" ns雪 ", u64::MAX, u64::MAX)));
    out.insert("slice_empty".into(), helper_state(Slice { limit: None, offset: u64::MAX }));
    out.insert("slice_zero".into(), helper_state(Slice { limit: Some(0), offset: 0 }));
    out.insert("aggregate_count_field".into(), helper_state(Aggregate::count_field(field, pattern)));
    out.insert("aggregate_sum".into(), helper_state(Aggregate::sum(field, pattern)));
    out.insert("aggregate_avg".into(), helper_state(Aggregate::avg(field, pattern)));
    out.insert("aggregate_min".into(), helper_state(Aggregate::min(field, pattern)));
    out.insert("aggregate_max".into(), helper_state(Aggregate::max(field, pattern)));
    out.insert("aggregate_stddev".into(), helper_state(Aggregate::stddev(field, pattern)));
    out.insert("aggregate_stddev_pop".into(), helper_state(Aggregate::stddev_pop(field, pattern)));
    out.insert("aggregate_var_samp".into(), helper_state(Aggregate::var_samp(field, pattern)));
    out.insert("aggregate_var_pop".into(), helper_state(Aggregate::var_pop(field, pattern)));
    out.insert("aggregate_bit_and".into(), helper_state(Aggregate::bit_and(field, pattern)));
    out.insert("aggregate_bit_or".into(), helper_state(Aggregate::bit_or(field, pattern)));
    out.insert("aggregate_bit_xor".into(), helper_state(Aggregate::bit_xor(field, pattern)));
    out.insert("constants".into(), json!([ContinuousPageFetchOptions::DEFAULT_TTL_SECONDS,
        IdSetPaginationOptions::DEFAULT_TTL_SECONDS, IdSetPaginationOptions::DEFAULT_MAX_IDS]));
    Json::Object(out)
}

fn validate_query(input: &Json) -> Json {
    let namespace = input["namespace"].as_str().unwrap();
    let ttl: u64 = input["ttl"].as_str().unwrap().parse().unwrap();
    let maximum: u64 = input["maximum"].as_str().unwrap().parse().unwrap();
    let outcome = std::panic::catch_unwind(|| match input["type"].as_str().unwrap() {
        "continuous" => { let _ = ContinuousPageFetchOptions::new(namespace, ttl); }
        "id_set" => { let _ = IdSetPaginationOptions::new(namespace, ttl, maximum); }
        "hard_limit" => { let _ = SelectQuery::new("Order").hard_limit(maximum); }
        _ => panic!("invalid validation fixture"),
    });
    match outcome {
        Ok(()) => json!({"ok":true}),
        Err(error) => json!({"error":error.downcast_ref::<String>().cloned()
            .or_else(||error.downcast_ref::<&str>().map(|v|(*v).to_owned())).unwrap()}),
    }
}

fn observe(input: Json) -> Json {
    if input["kind"] == "validate_query" { return validate_query(&input); }
    if input["kind"] == "query_helpers" { return query_helpers(&input); }
    if input["kind"] == "query_builders" { return query_builders(&input); }
    if input["kind"] == "full_expression" { return full_expression(&input); }
    let mut query = SelectQuery::new(input["entity"].as_str().unwrap());
    let mut states = vec![format!("{query:?}")];
    for names in input["projections"].as_array().unwrap() {
        query = query.projects(
            names
                .as_array()
                .unwrap()
                .iter()
                .map(|v| v.as_str().unwrap()),
        );
        states.push(format!("{query:?}"));
    }
    let cache =
        AggregationCacheOptions::enabled(input["expires"].as_str().unwrap().parse().unwrap())
            .propagate(input["propagate"].as_str().unwrap().parse().unwrap());
    query.aggregation_cache = Some(cache);
    states.push(format!("{query:?}"));
    let values = input["values"]
        .as_array()
        .unwrap()
        .iter()
        .map(fixture_value::value)
        .collect::<Vec<_>>();
    let parts = values
        .iter()
        .map(|v| Expr::eq("key", v.clone()))
        .collect::<Vec<_>>();
    let functions = [
        ExprFunction::Soundex,
        ExprFunction::Gbk,
        ExprFunction::Count,
        ExprFunction::Sum,
        ExprFunction::Avg,
        ExprFunction::Min,
        ExprFunction::Max,
        ExprFunction::Stddev,
        ExprFunction::StddevPop,
        ExprFunction::VarSamp,
        ExprFunction::VarPop,
        ExprFunction::BitAnd,
        ExprFunction::BitOr,
        ExprFunction::BitXor,
    ];
    json!({"queries":states,"cache":format!("{cache:?}"),
           "function":format!("{:?}",Expr::function(functions[input["function"].as_u64().unwrap() as usize],parts.clone())),
           "and":format!("{:?}",Expr::and(parts.clone())),"or":format!("{:?}",Expr::or(parts)),
           "in":format!("{:?}",Expr::in_list("key",values.clone())),
           "not_in":format!("{:?}",Expr::not_in_list("key",values.clone())),
           "large":format!("{:?}",Expr::in_large("key",values.clone())),
           "not_large":format!("{:?}",Expr::not_in_large("key",values))})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
