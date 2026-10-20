//! Actual frozen relationship requests, complete query Debug, metrics and SQL logs.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{Entity, Expr};
#[path = "../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;
// Controlled metadata representation experiment; production/reference algorithms stay unchanged.
fn raw_column_metadata() -> teaql_runtime::InMemoryMetadataStore {
    let mut metadata = cpp_dynamic_json_core::module().metadata;
    let mut descriptor = teaql_runtime::MetadataStore::entity(&metadata, "Line").unwrap().clone();
    let property = descriptor.properties.iter_mut().find(|property| property.name == "order_id").unwrap();
    assert_eq!(property.column_name, "\"order\"");
    property.column_name = "order".into();
    metadata.register(descriptor);
    metadata
}
fn native_value(value: &Json) -> teaql_core::Value {
    use teaql_core::Value;
    match value {
        Json::Null => Value::Null,
        Json::Bool(v) => Value::Bool(*v),
        Json::Number(v) => {
            if let Some(v) = v.as_i64() {
                Value::I64(v)
            } else {
                Value::F64(v.as_f64().unwrap())
            }
        }
        Json::String(v) => Value::Text(v.clone()),
        Json::Array(v) => Value::List(v.iter().map(native_value).collect()),
        Json::Object(v) => Value::Object(
            v.iter()
                .map(|(k, v)| (k.clone(), native_value(v)))
                .collect(),
        ),
    }
}
fn decode_observation(input: &Json) -> Json {
    let mut values = std::collections::BTreeMap::new();
    values.insert("id".to_owned(), teaql_core::Value::I64(10));
    values.insert("version".to_owned(), teaql_core::Value::I64(1));
    let relation = input["relation"].as_str().unwrap();
    if let Some(value) = input.get("payload") {
        values.insert(relation.to_owned(), native_value(value));
    }
    let root = teaql_runtime::EntityRuntimeState::default();
    match cpp_dynamic_json_core::Order::from_compact_row_with_context(
        teaql_core::CompactRow::from_map(values),
        &root,
    ) {
        Ok(entity) => {
            json!({"customer":entity.customer().map(|v|v.id()),"lines":entity.lines().value().map(|v|v.len()).unwrap_or(0),"lines_loaded":entity.lines().is_loaded(),"relation_loaded":entity.is_loaded(relation),"error":null})
        }
        Err(error) => json!({"error":error.to_string()}),
    }
}
fn observe(input: &Json) -> Json {
    if input["kind"] == "decode" {
        return decode_observation(input);
    }
    let alias = input["alias"].as_str().unwrap_or("metric");
    let cutoff = input["cutoff"].as_i64().unwrap_or(0);
    macro_rules! child {
        ($normal:expr, $minimal:expr) => {{
            let mut query = if input["child_minimal"] == true {
                $minimal
            } else {
                $normal
            };
            query = query.filter(Expr::gt("id", cutoff));
            if input["child_deleted"] == true {
                query = query.with_deleted_rows();
            }
            if input["child_slice"] == true {
                query = query.order_by_id_desc().limit(1).offset(1, 1);
            }
            if input["child_group"] == true {
                query = query.group_by("version").count_as("_child_count");
            }
            if input["child_options"] == true {
                query = query
                    .comment("child option comment")
                    .create_property_as("_custom", "42")
                    .raw_sql_filter("1=1");
            }
            query
        }};
    }
    macro_rules! finish {
        ($request:expr, $detail:expr) => {{
            let mut request = $request;
            if input["empty"] == true { request = request.filter(Expr::lt("id",0_i64)); }
            if input["deleted"] == true { request = request.with_deleted_rows(); }
            if input["outer_count"] == true { request = request.count_as("_outer_count"); }
            request = request.order_by_id_asc();
            let query:teaql_core::SelectQuery=request.clone().into();
            let mut out=json!({"query":format!("{query:?}")});
            if input["execute"] == true {
                let pool=std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
                pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
                pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
                let user_context=driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1).with_metadata(raw_column_metadata());
                let request=request.comment("inspect relationship analysis").purpose("qualify relationship analysis");
                let mode=input["mode"].as_str().unwrap_or("list");
                out["rows"]=json!([]);out["error"]=Json::Null;out["total"]=Json::Null;
                if mode == "rows" {
                    match driver::block_on(request.execute_for_rows(&user_context)) {
                        Ok(rows)=>out["rows"]=json!(rows.into_iter().map(|row|input["columns"].as_array().unwrap().iter().map(|key|row.get(key.as_str().unwrap()).map(|v|format!("{v:?}"))).collect::<Vec<_>>()).collect::<Vec<_>>()),
                        Err(error)=>out["error"]=json!(error.to_string()),
                    }
                } else {
                    let result = if mode=="one" {
                        driver::block_on(request.execute_for_one(&user_context)).map(|value|teaql_core::SmartList::from(value.into_iter().collect::<Vec<_>>()))
                    } else if mode=="page" { driver::block_on(request.execute_for_page(&user_context,1,2)) }
                    else { driver::block_on(request.execute_for_list(&user_context)) };
                    match result {
                        Ok(rows)=>{
                            out["total"]=json!(rows.total_count);
                            out["rows"]=json!(rows.into_iter().map(|entity|{
                                let detail=($detail)(&entity);
                                let id=entity.id();let version=entity.version();let values=entity.into_json_values();
                                json!({"id":id,"version":version,"detail":detail,"values":input["columns"].as_array().unwrap().iter().map(|key|values.get(key.as_str().unwrap()).map(|v|format!("{v:?}"))).collect::<Vec<_>>()})
                            }).collect::<Vec<_>>());
                        },
                        Err(error)=>out["error"]=json!(error.to_string()),
                    }
                }
                out["sql"]=json!(user_context.sql_logs().into_iter().map(|log|json!({"sql":log.sql,"params":log.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":log.comment,"purpose":log.purpose})).collect::<Vec<_>>());
            }
            out
        }};
    }
    if input["entity"] == "person" {
        let child = child!(Q::orders(), Q::orders_minimal());
        let mut query = if input["minimal"] == true {
            Q::people_minimal()
        } else {
            Q::people()
        };
        for action in input["actions"].as_array().unwrap() {
            query = match action.as_str().unwrap() {
                "have" => query.have_orders(),
                "have_no" => query.have_no_orders(),
                "matching" => query.with_orders_matching(child.clone()),
                "without" => query.without_orders_matching(child.clone()),
                "count" => query.count_orders(),
                "count_as" => query.count_orders_as(alias),
                "count_with" => query.count_orders_with(alias, child.clone()),
                "stats" => query.stats_from_orders(child.clone()),
                "stats_as" => query.stats_from_orders_as(alias, child.clone()),
                "group_details" => query.group_by_orders_with_details(child.clone()),
                "sum" => query.sum_amount_of_orders(),
                "sum_as" => query.sum_amount_of_orders_as(alias, child.clone()),
                "avg" => query.avg_amount_of_orders(),
                "avg_as" => query.avg_amount_of_orders_as(alias, child.clone()),
                "min" => query.min_amount_of_orders(),
                "min_as" => query.min_amount_of_orders_as(alias, child.clone()),
                "max" => query.max_amount_of_orders(),
                "max_as" => query.max_amount_of_orders_as(alias, child.clone()),
                "standard_deviation" => query.standard_deviation_amount_of_orders(),
                "standard_deviation_as" => {
                    query.standard_deviation_amount_of_orders_as(alias, child.clone())
                }
                "square_root_of_population_standard_deviation" => {
                    query.square_root_of_population_standard_deviation_amount_of_orders()
                }
                "square_root_of_population_standard_deviation_as" => query
                    .square_root_of_population_standard_deviation_amount_of_orders_as(
                        alias,
                        child.clone(),
                    ),
                "sample_variance" => query.sample_variance_amount_of_orders(),
                "sample_variance_as" => {
                    query.sample_variance_amount_of_orders_as(alias, child.clone())
                }
                "sample_population_variance" => query.sample_population_variance_amount_of_orders(),
                "sample_population_variance_as" => {
                    query.sample_population_variance_amount_of_orders_as(alias, child.clone())
                }
                "have_profile" => query.have_profile(),
                "have_no_profile" => query.have_no_profile(),
                _ => panic!("unknown person action"),
            };
        }
        finish!(query, |_: &cpp_dynamic_json_core::Person| Json::Null)
    } else {
        let child = child!(Q::lines(), Q::lines_minimal());
        let customer = child!(Q::people(), Q::people_minimal());
        let mut query = if input["minimal"] == true {
            Q::orders_minimal()
        } else {
            Q::orders()
        };
        for action in input["actions"].as_array().unwrap() {
            query = match action.as_str().unwrap() {
                "have" => query.have_lines(),
                "have_no" => query.have_no_lines(),
                "matching" => query.with_lines_matching(child.clone()),
                "without" => query.without_lines_matching(child.clone()),
                "count" => query.count_lines(),
                "count_as" => query.count_lines_as(alias),
                "count_with" => query.count_lines_with(alias, child.clone()),
                "stats" => query.stats_from_lines(child.clone()),
                "stats_as" => query.stats_from_lines_as(alias, child.clone()),
                "group_details" => query.group_by_lines_with_details(child.clone()),
                "sum" => query.sum_quantity_of_lines(),
                "sum_as" => query.sum_quantity_of_lines_as(alias, child.clone()),
                "avg" => query.avg_quantity_of_lines(),
                "avg_as" => query.avg_quantity_of_lines_as(alias, child.clone()),
                "min" => query.min_quantity_of_lines(),
                "min_as" => query.min_quantity_of_lines_as(alias, child.clone()),
                "max" => query.max_quantity_of_lines(),
                "max_as" => query.max_quantity_of_lines_as(alias, child.clone()),
                "standard_deviation" => query.standard_deviation_quantity_of_lines(),
                "standard_deviation_as" => {
                    query.standard_deviation_quantity_of_lines_as(alias, child.clone())
                }
                "square_root_of_population_standard_deviation" => {
                    query.square_root_of_population_standard_deviation_quantity_of_lines()
                }
                "square_root_of_population_standard_deviation_as" => query
                    .square_root_of_population_standard_deviation_quantity_of_lines_as(
                        alias,
                        child.clone(),
                    ),
                "sample_variance" => query.sample_variance_quantity_of_lines(),
                "sample_variance_as" => {
                    query.sample_variance_quantity_of_lines_as(alias, child.clone())
                }
                "sample_population_variance" => {
                    query.sample_population_variance_quantity_of_lines()
                }
                "sample_population_variance_as" => {
                    query.sample_population_variance_quantity_of_lines_as(alias, child.clone())
                }
                "have_customer" => query.have_customer(),
                "have_no_customer" => query.have_no_customer(),
                "matching_customer" => query.with_customer_matching(customer.clone()),
                "without_customer" => query.without_customer_matching(customer.clone()),
                "group_customer" => query.group_by_customer_with(customer.clone()),
                "group_customer_details" => query.group_by_customer_with_details(),
                "group_customer_from" => {
                    query.group_by_customer_with_details_from(customer.clone())
                }
                "roll_customer" => query.roll_up_to_customer(),
                "roll_customer_with" => query.roll_up_to_customer_with(customer.clone()),
                _ => panic!("unknown order action"),
            };
        }
        finish!(query, |entity: &cpp_dynamic_json_core::Order| {
            entity.customer().map(|customer|json!({"id":customer.id(),"name":if customer.is_loaded("name"){Some(customer.name())}else{None}}))
        })
    }
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let out=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||observe(&input))).unwrap_or_else(|error|json!({"panic":if let Some(v)=error.downcast_ref::<String>(){v.clone()}else{error.downcast_ref::<&str>().unwrap().to_string()}}));
        println!("{out}");
    }
}
