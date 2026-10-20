// Appended only to a byte-checked runtime view, inside the fixture module.
use super::PurposedSelectQuery;
use teaql_core::request::{QueryOptions, QuerySelection};

fn source_leaf() -> SelectQuery {
    let mut query = SelectQuery::new("Leaf").filter(Expr::eq("name", "private leaf"));
    query.facets.push(FacetRequest::new(
        "leaf facet", "unused", SelectQuery::new("Terminal"), true));
    query
}

fn source_traversal(shape: u64) -> SelectQuery {
    let mut query = SelectQuery::new("Root");
    let leaf = source_leaf();
    if shape == 1 || shape == 8 {
        query.facets.push(FacetRequest::new("direct", "unused", leaf.clone(), false));
    }
    if shape == 2 || shape == 8 { query = query.relation_query("branch", leaf.clone()); }
    if shape == 3 || shape == 8 { query = query.relation("unqueried"); }
    if shape == 4 || shape == 8 { query = query.child_enhancement(leaf.clone()); }
    if shape == 5 || shape == 8 { query = query.object_group_by("object", "id", leaf.clone()); }
    if shape == 6 || shape == 8 {
        query = query.filter(Expr::in_subquery("id", EntityDescriptor::new("Leaf"), leaf.clone(), "id"));
    }
    if shape == 7 || shape == 8 {
        query.relation_aggregates.push(RelationAggregate::new("unused", "core aggregate", leaf, false));
    }
    query
}

fn source_options() -> QueryOptions {
    let mut selection = QuerySelection::new(SelectQuery::new("Selection"));
    selection.query_options.comment = Some("selection metadata".into());
    selection.query_options.raw_sql = Some("trusted diagnostic metadata".into());
    selection.query_options.raw_sql_search_criteria.push("selection criterion".into());
    selection.child_enhancements.push(QuerySelection::new(source_leaf()));
    selection.query_options.relation_aggregates.push(teaql_core::request::RelationAggregate::new(
        "aggregate edge", "builder aggregate", QuerySelection::new(source_leaf()), false));
    selection.query_options.facets.push(FacetRequest::new(
        "nested", "nested edge", source_leaf(), false));
    let mut options = QueryOptions::default();
    options.facets.push(FacetRequest::new("future", "future edge", selection, true));
    options
}

fn source_state(query: &PurposedSelectQuery) -> Json {
    json!({"query":format!("{:?}", query.as_query()),
           "diagnostic":query.diagnostic_source().map(|source|format!("{source:?}")),
           "wrapper":format!("{query:?}")})
}

fn source_observe(input: &Json) -> Json {
    let mut base = if input["kind"] == "full" {
        make(input["profile"].as_u64().unwrap() as usize)
    } else { source_traversal(input["shape"].as_u64().unwrap()) };
    if input["kind"] == "full" && input["field"] != 31 {
        mutate(&mut base, input["field"].as_u64().unwrap() as usize);
    }
    let original = base.clone();
    let detected = super::query_has_facets(&base);
    let mut expanded = base.clone();
    super::expand_query_facet_diagnostics(&mut expanded);
    let options = source_options();
    let selection_source = super::facet_diagnostic_source(&options.facets[0].query);
    let mut query = PurposedSelectQuery::new(base.clone(), " source purpose \t");
    let mut states = vec![source_state(&query)];
    for step in input["steps"].as_array().unwrap() {
        query = match step.as_str().unwrap() {
            "count" => query.for_exact_count("count"),
            "facets" => query.with_facet_diagnostics(&options),
            "empty" => query.with_facet_diagnostics(&QueryOptions::default()),
            "clone" => query.clone(),
            _ => panic!("Unknown source step"),
        };
        states.push(source_state(&query));
    }
    let copy = query.clone();
    json!({"base":format!("{base:?}"),"has_facets":detected,
           "expanded":format!("{expanded:?}"),"selection_source":format!("{selection_source:?}"),
           "states":states,"copy":source_state(&copy),
           "exported":format!("{:?}",copy.into_query()),"base_unchanged":base==original})
}

#[test]
fn observe_private_sources() {
    let input = std::fs::read_to_string(std::env::var("TEAQL_SUPPORT_SOURCE_INPUTS").unwrap()).unwrap();
    let values: Vec<Json> = serde_json::from_str(&input).unwrap();
    let observed: Vec<Json> = values.iter().map(source_observe).collect();
    std::fs::write(std::env::var("TEAQL_SUPPORT_SOURCE_OUTPUTS").unwrap(),
                   serde_json::to_vec(&observed).unwrap()).unwrap();
}

#[test]
fn source_interface_contracts() {
    fn query_services<E, T>(service: &crate::EntityDataService<'_, E>,
                            query: &PurposedSelectQuery,
                            aggregates: &[teaql_core::RelationAggregate])
    where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor
             + teaql_data_service::StreamQueryExecutor + Send + Sync + 'static,
          T: teaql_core::Entity {
        let _ = super::TeaqlQueryDataService::fetch_all(service, query);
        let _ = super::TeaqlQueryDataService::fetch_smart_list(service, query);
        let _ = super::TeaqlQueryDataService::fetch_smart_list_with_relation_aggregates(service, query, aggregates);
        let _ = super::TeaqlQueryDataService::fetch_stream(service, query);
        let _ = super::TeaqlEntityDataService::fetch_enhanced_entities::<T>(service, query);
        let _ = super::TeaqlEntityDataService::fetch_enhanced_entities_with_relation_aggregates::<T>(service, query, aggregates);
    }
    fn error_contract<R: super::TeaqlQueryDataService>() {
        fn requires_error<E: std::error::Error + Send + Sync + 'static>() {}
        requires_error::<R::Error>();
        fn alias<R: super::TeaqlQueryDataService>(value: super::TeaqlDataServiceError<R>)
            -> crate::DataServiceError<R::Error> { value }
    }
    fn consuming_save<'a, C: super::TeaqlRuntime + 'a, S: super::AuditedSave<'a, C> + 'a>(
        save: S, context: &'a C) {
        let _ = save.save(context);
    }
}
