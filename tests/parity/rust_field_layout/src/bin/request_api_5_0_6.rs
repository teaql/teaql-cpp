use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::*;
use teaql_core::request as request;
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}

#[derive(Debug, PartialEq)]
struct NonClone(i64);
struct CustomId(Value);
impl request::EntityReference for CustomId {
    fn entity_id_value(self) -> Value { self.0 }
}

fn evaluate(input: Json) -> Json {
    let field=input["field"].as_str().unwrap_or("same");
    let leaf=request::QuerySelection::new(SelectQuery::new("Child"));
    macro_rules! observe_equality {
        ($before:expr,$after:expr) => {
            json!({"equal":$before==$after,"before":format!("{:?}",$before),"after":format!("{:?}",$after)})
        };
    }
    macro_rules! relation_equality {
        ($kind:ty) => {{
            let before=<$kind>::new("child",leaf);
            let mut after=before.clone();
            match field {
                "same"=>(),
                "name"=>after.name="changed".into(),
                "query"=>after.query.entity="Changed".into(),
                "relation_selections"=>after.relation_selections.push(request::RelationSelection::new("nested",request::QuerySelection::new(SelectQuery::new("Child")))),
                "relation_filters"=>after.relation_filters.push(request::RelationFilter::new("nested",request::QuerySelection::new(SelectQuery::new("Child")))),
                "child_enhancements"=>after.child_enhancements.push(request::QuerySelection::new(SelectQuery::new("Child"))),
                "query_options"=>after.query_options.comment=Some(String::new()),
                _=>panic!("Unknown relation field"),
            }
            observe_equality!(before,after)
        }};
    }
    match input["model"].as_str().unwrap_or("") {
        "relation_eq"=>return relation_equality!(request::RelationSelection),
        "filter_eq"=>return relation_equality!(request::RelationFilter),
        "aggregate_eq"=> {
            let before=request::RelationAggregate::new("child","count",leaf,false);
            let mut after=before.clone();
            match field {
                "same"=>(),
                "relation_name"=>after.relation_name="changed".into(),
                "alias"=>after.alias="changed".into(),
                "query"=>after.query.query.entity="Changed".into(),
                "single_result"=>after.single_result=true,
                _=>panic!("Unknown aggregate field"),
            }
            return observe_equality!(before,after);
        },
        "facet_eq"=> {
            let before=request::FacetRequest::new("facet","child",leaf,false);
            let mut after=before.clone();
            match field {
                "same"=>(),
                "facet_name"=>after.facet_name="changed".into(),
                "relation_name"=>after.relation_name="changed".into(),
                "query"=>after.query.query.entity="Changed".into(),
                "include_all_facets"=>after.include_all_facets=true,
                _=>panic!("Unknown facet field"),
            }
            return observe_equality!(before,after);
        },
        "group_eq"=> {
            let before=request::ObjectGroupBy::new("field","column",leaf);
            let mut after=before.clone();
            match field {
                "same"=>(),
                "property_name"=>after.property_name="changed".into(),
                "storage_field"=>after.storage_field="changed".into(),
                "query"=>after.query.query.entity="Changed".into(),
                _=>panic!("Unknown grouping field"),
            }
            return observe_equality!(before,after);
        },
        "dynamic_eq"|"projection_eq"=> {
            macro_rules! raw_equality {
                ($kind:ty)=>{{
                    let before=<$kind>::new("field",request::UnsafeRawSqlSegment::trusted("raw"));
                    let mut after=before.clone();
                    match field {
                        "same"=>(),
                        "property_name"=>after.property_name="changed".into(),
                        "raw_sql_segment"=>after.raw_sql_segment="changed".into(),
                        _=>panic!("Unknown raw projection field"),
                    }
                    observe_equality!(before,after)
                }};
            }
            return if input["model"]=="dynamic_eq" {
                raw_equality!(request::RawDynamicProperty)
            } else { raw_equality!(request::RawProjection) };
        },
        "range_eq"=> {
            let before=request::DateRange::new(Value::U64(1),Value::U64(2));
            let mut after=before.clone();
            match field {
                "same"=>(),
                "start"=>after.start=Value::U64(3),
                "end"=>after.end=Value::U64(3),
                _=>panic!("Unknown range field"),
            }
            return observe_equality!(before,after);
        },
        "trusted_eq"=> {
            let before=request::UnsafeRawSqlSegment::trusted("raw");
            let after=request::UnsafeRawSqlSegment::trusted(if field=="sql" {"changed"} else {"raw"});
            assert!(field=="sql"||field=="same");
            return observe_equality!(before,after);
        },
        _=>(),
    }
    if input["model"]=="options_eq" {
        let before=request::QueryOptions::default();
        let mut after=before.clone();
        let leaf=request::QuerySelection::new(SelectQuery::new("Child"));
        match input["field"].as_str().unwrap() {
            "same" => (),
            "comment" => after.comment=Some(String::new()),
            "raw_sql" => after.raw_sql=Some(String::new()),
            "raw_sql_search_criteria" => after.raw_sql_search_criteria.push(String::new()),
            "dynamic_properties" => after.dynamic_properties.push(request::RawDynamicProperty::new("x",request::UnsafeRawSqlSegment::trusted(""))),
            "raw_projections" => after.raw_projections.push(request::RawProjection::new("x",request::UnsafeRawSqlSegment::trusted(""))),
            "relation_aggregates" => after.relation_aggregates.push(request::RelationAggregate::new("child","count",leaf,true)),
            "object_group_bys" => after.object_group_bys.push(request::ObjectGroupBy::new("field","column",leaf)),
            "facets" => after.facets.push(request::FacetRequest::new("facet","child",leaf,false)),
            _=>panic!("Unknown option field"),
        }
        return json!({"equal":before==after,"before":format!("{before:?}"),"after":format!("{after:?}")});
    }
    if input["model"]=="selection_eq" {
        let before=request::QuerySelection::new(SelectQuery::new("Parent"));
        let mut after=before.clone();
        let leaf=request::QuerySelection::new(SelectQuery::new("Child"));
        match input["field"].as_str().unwrap() {
            "same" => (),
            "query" => after.query.entity="Changed".into(),
            "relation_selections" => after.relation_selections.push(request::RelationSelection::new("child",leaf)),
            "relation_filters" => after.relation_filters.push(request::RelationFilter::new("child",leaf)),
            "child_enhancements" => after.child_enhancements.push(leaf),
            "query_options" => after.query_options.comment=Some(String::new()),
            _=>panic!("Unknown selection field"),
        }
        return json!({"equal":before==after,"before":format!("{before:?}"),"after":format!("{after:?}")});
    }
    let flags=input["flags"].as_u64().unwrap();
    let payload=fixture_value::value(&input["value"]);
    let range=request::DateRange::new(payload.clone(),Value::Timestamp(teaql_core::time::Timestamp(-1)));
    let nonclone=request::DateRange::new(NonClone(-1),NonClone(i64::MAX));
    let nonclone_vector=request::DateRange::new(vec![NonClone(3)],vec![NonClone(4)]);
    let trusted=request::UnsafeRawSqlSegment::trusted("raw\n雪\0");
    let dynamic=request::RawDynamicProperty::new("dynamic",trusted.clone());
    let projection=request::RawProjection::new("project",trusted.clone());
    let mut child=SelectQuery::new("Child");
    child.projection=vec!["id".into(),"id".into()];
    child.filter=Some(Expr::eq("private",payload.clone()));
    let leaf=request::QuerySelection::new(child.clone());
    let relation=request::RelationSelection::new("children",leaf.clone());
    let relation_filter=request::RelationFilter::new("children",leaf.clone());
    let aggregate=request::RelationAggregate::new("children","amount",
        SelectQuery::new(if flags & 1 != 0 {"Order"} else {"Other"}),flags & 2 != 0);
    let facet=request::FacetRequest::new("status","children",leaf.clone(),flags & 2 != 0);
    let group=request::ObjectGroupBy::new("group","group_id",leaf.clone());
    let mut options=request::QueryOptions::default();
    if flags & 4 != 0 {
        options.comment=Some(if flags & 1 != 0 {String::new()} else {"reason\n雪".into()});
        options.raw_sql=Some(String::new());options.raw_sql_search_criteria=vec!["a".into(),"a".into()];
        options.dynamic_properties=vec![dynamic.clone()];options.raw_projections=vec![projection.clone()];
        options.relation_aggregates=vec![aggregate.clone()];options.object_group_bys=vec![group.clone()];
        options.facets=vec![facet.clone()];
    }
    let outer=SelectQuery::new("Order").filter(Expr::eq("outer",payload.clone()));
    let mut selection=request::QuerySelection::new(outer.clone());
    selection.query_options=options.clone();
    if flags & 2 != 0 {
        selection.relation_selections=vec![relation.clone()];
        selection.relation_filters=vec![relation_filter.clone()];selection.child_enhancements=vec![leaf.clone()];
    }
    let mut nested_relation=request::RelationSelection::new("nested",selection.clone());
    nested_relation.query_options=options.clone();
    let mut merged=selection.clone();
    request::merge_outer_filter_into_facet_aggregates(&mut merged,&outer);
    let runtime=request::runtime_relation_aggregates(&options);
    let converted=selection.clone().into_query();
    let applied=request::apply_runtime_metadata(outer.clone(),&options,&selection.child_enhancements);
    let relations=request::apply_relation_selections(outer.clone(),selection.relation_selections.clone());
    let mut output=json!({"range":formatted(&range),"nonclone_range":formatted(&nonclone),
        "nonclone_vector_range":formatted(&nonclone_vector),
        "trusted":formatted(&trusted),"trusted_sql":trusted.clone().into_sql(),
        "dynamic":formatted(&dynamic),"projection":formatted(&projection),"options":formatted(&options),
        "relation":formatted(&relation),"relation_filter":formatted(&relation_filter),
        "aggregate":formatted(&aggregate),"facet":formatted(&facet),"group":formatted(&group),
        "selection":formatted(&selection),"nested_relation":formatted(&nested_relation),
        "converted":formatted(&converted),"relation_converted":formatted(&nested_relation.into_query()),
        "applied":formatted(&applied),"relations_applied":formatted(&relations),"merged":formatted(&merged),
        "runtime_aggregates":runtime.iter().map(formatted).collect::<Vec<_>>(),
        "clone_equal":selection==selection.clone(),
        "ids":[format!("{:?}",request::EntityReference::entity_id_value(payload.clone())),
            format!("{:?}",request::EntityReference::entity_id_value(u64::MAX)),
            format!("{:?}",request::EntityReference::entity_id_value(CustomId(payload.clone())))],
        "constants":[request::COUNT_ALIAS,request::TYPE_FIELD,request::TYPE_GROUP_FIELD]});
    if input["enums"]==true {
        use request::FieldOperator::*;
        output["operators"]=[Equal,NotEqual,GreaterThan,GreaterThanOrEqual,LessThan,LessThanOrEqual,
            Between,In,NotIn,Contain,NotContain,BeginWith,NotBeginWith,EndWith,NotEndWith,SoundsLike,IsNull,IsNotNull]
            .iter().map(formatted).collect();
    }
    output
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
