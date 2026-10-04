use std::{env,fs,path::PathBuf};
fn main(){
    let root=PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap()).join("../../../.reference/teaql-rs/teaql-runtime/src");
    let out=PathBuf::from(env::var("OUT_DIR").unwrap());
    for (file,start,end,target,prefix,suffix) in [
        ("mutation_policy.rs","tokio::task_local! {","\nimpl UserContext {","governance_scope.rs","",""),
        ("context.rs","    pub fn send_event(","    pub async fn get_in_store(","context_event.rs","impl UserContext {\n","}\n")]{
        let path=root.join(file);println!("cargo:rerun-if-changed={}",path.display());
        let text=fs::read_to_string(path).unwrap();let from=text.find(start).expect("locked source start marker");let to=from+text[from..].find(end).expect("locked source end marker");
        fs::write(out.join(target),format!("{prefix}{}{suffix}",&text[from..to])).unwrap();
    }
    // Compile the locked private planning functions verbatim. The probe only
    // supplies a minimal field wrapper around a real public UserContext.
    let mut planning=String::new();
    for (file,start,end) in [
        ("data_service/relation.rs","#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]","#[derive(Debug, Clone, Copy, PartialEq, Eq)]"),
        ("data_service/helpers.rs","pub(super) fn relation_bucket_key(","pub(super) fn aggregation_cache_namespace("),
        ("data_service/helpers.rs","pub(super) fn graph_identity_key(","pub(super) fn ensure_relation_target<"),
        ("data_service/helpers.rs","pub(super) fn ensure_projection(","pub(super) fn attach_empty_relation_aggregate(")
    ] {
        let path=root.join(file);println!("cargo:rerun-if-changed={}",path.display());let text=fs::read_to_string(path).unwrap();let from=text.find(start).expect("planning start");let to=from+text[from..].find(end).expect("planning end");planning.push_str(&text[from..to]);
    }
    planning.push_str("impl<'a> PlanningRepo<'a> {\n");
    for (start,end) in [
        ("    fn build_relation_plans(","    fn enhance_plan<'b>"),
        ("    fn query_for_plan(","    async fn fetch_relation_rows("),
        ("    fn ensure_stable_top_n_order(","    fn attach_relation_rows(")
    ] {
        let text=fs::read_to_string(root.join("data_service/relation.rs")).unwrap();let from=text.find(start).expect("planning method start");let to=from+text[from..].find(end).expect("planning method end");planning.push_str(&text[from..to]);
    }
    planning.push_str("}\n");fs::write(out.join("relation_planning.rs"),planning).unwrap();

    let path=root.join("data_service/base.rs");println!("cargo:rerun-if-changed={}",path.display());let text=fs::read_to_string(path).unwrap();let start=text.find("    pub(crate) async fn fetch_all(").unwrap();let end=start+text[start..].find("    pub(crate) async fn insert(").unwrap();
    fs::write(out.join("runtime_base_query.rs"),format!("impl<'a,M,E> RuntimeDataService<'a,M,E> where M: MetadataStore,E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Sync {{\n{}\n}}",&text[start..end])).unwrap();

    let path=root.join("data_service/context.rs");println!("cargo:rerun-if-changed={}",path.display());let text=fs::read_to_string(path).unwrap();
    let start=text.find("    async fn observe<T, F, N>(").unwrap();let end=start+text[start..].find("    pub(crate) async fn insert(").unwrap();
    let comment=text.find("    pub(crate) fn resolve_final_comment(").unwrap();
    fs::write(out.join("runtime_context_query.rs"),format!("impl<'a,E> ContextDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {{\n{}\n{}",&text[start..end],&text[comment..])).unwrap();

    let text=fs::read_to_string(root.join("data_service/relation.rs")).unwrap();let start=text.find("#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]").unwrap();let end=text.find("impl<'a, E> EntityDataService<'a, E>").unwrap();
    let mut execution=text[start..end].to_owned();
    for (start,end) in [("pub(super) fn relation_bucket_key(","pub(super) fn aggregation_cache_namespace("),("pub(super) fn graph_identity_key(","pub(super) fn ensure_relation_target<"),("pub(super) fn ensure_projection(","pub(super) fn attach_empty_relation_aggregate(")]{
        let helpers=fs::read_to_string(root.join("data_service/helpers.rs")).unwrap();let from=helpers.find(start).unwrap();let to=from+helpers[from..].find(end).unwrap();execution.push_str(&helpers[from..to]);
    }
    execution.push_str("impl<'a,E> EntityDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n");
    let start=text.find("    fn query_for_plan(").unwrap();let end=text[start..].find("    fn attach_relation_rows(").unwrap()+start;execution.push_str(&text[start..end]);execution.push_str("}\n");
    fs::write(out.join("relation_execution.rs"),&execution).unwrap();
    let mut hydration=execution;
    hydration.push_str("impl<'a,E> EntityDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n");
    let start=text.find("    pub(crate) async fn hydrate_flat_plans_internal(").unwrap();let end=text[start..].find("    pub(crate) fn flat_relation_plans(").unwrap()+start;hydration.push_str(&text[start..end]);
    let start=text.find("    fn enhance_plan<'b>(").unwrap();let end=text[start..].find("    fn query_for_plan(").unwrap()+start;hydration.push_str(&text[start..end]);
    let start=text.find("    fn attach_relation_rows(").unwrap();let end=text[start..].find("\n}\n\n#[cfg(test)]").unwrap()+start;hydration.push_str(&text[start..end]);hydration.push_str("}\n");
    fs::write(out.join("relation_hydration.rs"),hydration).unwrap();

    let helpers=fs::read_to_string(root.join("data_service/helpers.rs")).unwrap();let mut enhancement=String::new();
    for (start,end) in [
        ("pub(super) fn default_aggregate_value(","pub(super) fn aggregation_cache_namespace("),
        ("pub(super) fn graph_identity_key(","pub(super) fn ensure_relation_target<"),
        ("pub(super) fn ensure_projection(","#[cfg(test)]\nmod aggregation_cache_key_tests"),
        ("pub(super) fn attach_relation_aggregate_rows(","pub(super) fn ensure_initial_version(")]{
        let start=helpers.find(start).unwrap();let end=start+helpers[start..].find(end).unwrap();enhancement.push_str(&helpers[start..end]);
    }
    enhancement.push_str("impl<'a,E> EntityDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n");
    for (start,end) in [("    pub(crate) fn enhance_relation_aggregates_internal<'b>(","    fn build_relation_plans("),("    fn build_relation_plans_from_loads(","    fn enhance_plan<'b>("),("    fn relation_child_repo(","    fn query_for_plan(")]{
        let start=text.find(start).unwrap();let end=start+text[start..].find(end).unwrap();enhancement.push_str(&text[start..end]);
    }
    enhancement.push_str("}\n");fs::write(out.join("relation_enhancement.rs"),enhancement).unwrap();

    let path=root.join("context/pagination.rs");println!("cargo:rerun-if-changed={}",path.display());let pagination=fs::read_to_string(path).unwrap();
    let end=pagination.find("pub(super) fn id_set_build_lock(").unwrap();fs::write(out.join("query_stores.rs"),&pagination[..end]).unwrap();
    let start=end;let end=start+pagination[start..].find("\nimpl UserContext {").unwrap();
    fs::write(out.join("id_set_build_lock.rs"),format!("use std::{{collections::HashMap,sync::{{Arc,Mutex,OnceLock}}}};\n{}",&pagination[start..end])).unwrap();
    let mut observations=String::from("impl UserContext {\n");
    for(start,end)in[("    pub fn continuous_page_plan(","    pub(crate) fn continuous_page_cursor_store("),("    pub fn id_set_plan(","    pub(crate) fn id_set_store(")]{let start=pagination.find(start).unwrap();let end=start+pagination[start..].find(end).unwrap();observations.push_str(&pagination[start..end]);}
    observations.push_str("}\n");fs::write(out.join("pagination_observations.rs"),observations).unwrap();
    let path=root.join("data_service/cache.rs");println!("cargo:rerun-if-changed={}",path.display());fs::write(out.join("aggregation_cache.rs"),fs::read_to_string(path).unwrap()).unwrap();

}
