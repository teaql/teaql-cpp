"""Build actual locked private save methods with append-only test access bridges."""

import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import re
import subprocess
import tarfile

from build_graph_fix_oracle import CONTEXT_BRIDGE, SAVE_BRIDGE
from build_graph_execution_oracle import BRIDGE as GRAPH_BRIDGE, NODE_BRIDGE, NODE_EXPORTS
from build_ledger_execution_oracle import BRIDGE as LEDGER_EXECUTION_BRIDGE
from build_ledger_plan_oracle import BRIDGE as LEDGER_BRIDGE
from build_policy_plan_oracle import BRIDGE as POLICY_BRIDGE
from build_save_preparation_oracle import BRIDGE as PREPARATION_BRIDGE
from build_runtime_mutation_oracle import BRIDGE as MUTATION_BRIDGE
from build_graph_saver_oracle import BRIDGE as SAVER_BRIDGE
from build_id_set_oracle import BRIDGE as ID_SET_PAGE_BRIDGE


ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "tests/parity/rust_private_save_5_0_6"
BUILD = ROOT / "build_rust_private_save_5_0_6"
SOURCE = BUILD / "original"
FAMILIES = ["graph_fix", "save_preparation", "ledger_plan", "policy_plan",
            "graph_execution", "ledger_execution", "runtime_mutation", "graph_saver",
            "audited_auto_save", "transaction_scope", "entity_query", "entity_stream",
            "enhanced_entities", "entity_decoder", "id_set_page", "entity_query_capture", "runtime_base", "context_query",
            "runtime_base_intent", "context_query_intent", "entity_service", "relation_planning_extensions", "relation_execution", "relation_enhancement",
            "relation_enhancement_variants", "hydration_sibling_keys", "hydration_owner_facets", "hydration_owner_facets_async", "hydration_flat_facets", "loaded_row_graph", "relation_hydration", "json_graph_views", "json_graph_decoder"]
LEDGER_EXECUTION_BRIDGE = LEDGER_EXECUTION_BRIDGE.replace(
    ".execute_ledger_plan_internal(root,locations).await",
    ".execute_ledger_plan_internal(root,locations).await.map(|outcome| {\n"
    "            assert!(outcome.dynamic_readbacks.is_none(),\n"
    "                \"historical observer cannot omit dynamic readbacks\");\n"
    "            outcome.generated_ids\n"
    "        })")
EXPORTS = """
pub use entity_save::{__teaql_cpp_fix_scope, __teaql_cpp_install_save_hook,
    __teaql_cpp_extract, __teaql_cpp_preflight, __teaql_cpp_merge,
    __teaql_cpp_hydrate, __teaql_cpp_locations, __teaql_cpp_cancelled,
    __teaql_cpp_preserve};
pub use data_service::__cpp_order_ledger;
""" + POLICY_BRIDGE + """
pub use data_service::{__cpp_execute_graph, __cpp_save_graph, __cpp_current_row,
    __cpp_save_entity_adapter, __cpp_save_core_graph, __cpp_execute_ledger,
""" + NODE_EXPORTS + "};\n"
EXPORTS += "\npub use data_service::{__CppMutation, __cpp_mutation};\n"
EXPORTS += "\npub use entity_save::__cpp_registered_save;\n"
RELATION_PLANNING_BRIDGE = """
impl<'a, E> EntityDataService<'a, E>
where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync,
{
    #[doc(hidden)]
    pub async fn __cpp_hydrate_historical(&self, mode: &str, parents: &mut [CompactRow],
        plans: &[RelationLoadPlan], root: &crate::EntityRuntimeState,
        graph: &mut crate::EntityGraphBuilder, children: Vec<CompactRow>)
        -> Result<(), DataServiceError<E::Error>> {
        match mode {
            "attach" => {
                let keys = relation_key_rows(parents, plans);
                let attachment = children.iter().map(|row|row.get(&plans[0].foreign_key).cloned()).collect();
                self.attach_relation_rows(parents, &keys, &plans[0], children, attachment);
                Ok(())
            },
            "install" => self.install_compact_flat_relation(parents, &plans[0], children, root, graph).await,
            "embedded" => {
                let keys = relation_key_rows(parents, plans);
                for plan in plans { self.enhance_plan(parents, &keys, plan).await?; }
                Ok(())
            },
            "flat" => self.hydrate_flat_plans_internal(parents, plans, root, graph).await,
            _ => self.hydrate_compact_flat_plans_internal(parents, plans, root, graph).await,
        }
    }
    #[doc(hidden)]
    pub async fn __cpp_flat_query(&self, parents: &mut [CompactRow], query: &SelectQuery,
        root: &crate::EntityRuntimeState, graph: &mut crate::EntityGraphBuilder, compact: bool)
        -> Result<(), DataServiceError<E::Error>> {
        let plans = self.build_relation_plans_from_loads(&query.entity, &query.relations).map_err(DataServiceError::Runtime)?;
        if compact { self.hydrate_compact_flat_plans_internal(parents, &plans, root, graph).await }
        else { self.hydrate_flat_plans_internal(parents, &plans, root, graph).await }
    }
    #[doc(hidden)]
    pub async fn __cpp_embedded_query(&self, parents: &mut [CompactRow], query: &SelectQuery)
        -> Result<(), DataServiceError<E::Error>> {
        self.enhance_query_relations_internal(parents, query).await
    }
    #[doc(hidden)]
    pub fn __cpp_relation_plans(&self, loads: &[String], query: Option<&SelectQuery>)
        -> Result<Vec<RelationLoadPlan>, RuntimeError> {
        match query {
            Some(query) => self.build_relation_plans_from_loads(&query.entity, &query.relations),
            None => self.build_relation_plans("Root", loads),
        }
    }
    #[doc(hidden)]
    pub fn __cpp_relation_base_query(&self, plan: &RelationLoadPlan) -> SelectQuery {
        self.base_relation_query(plan)
    }
    #[doc(hidden)]
    pub fn __cpp_relation_plan_query(&self, plan: &RelationLoadPlan,
        rows: &[CompactRow], compact: bool) -> SelectQuery {
        if compact { self.query_for_compact_plan(plan, rows) }
        else { self.query_for_plan(plan, rows) }
    }
}
"""
LOADED_ROW_GRAPH_BRIDGE = """
impl<'a, E> EntityDataService<'a, E>
where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync,
{
    #[doc(hidden)]
    pub fn __cpp_reconstruct_loaded_graph(&self, entity: &str, rows: &mut [CompactRow])
        -> Result<crate::EntityRuntimeState, teaql_core::EntityError> {
        self.attach_flat_relation_graph(entity, rows)
    }
}
"""
JSON_GRAPH_VIEWS_BRIDGE = """
impl crate::EntityGraphBuilder {
    #[doc(hidden)] pub fn __cpp_for_json_read() -> Self { Self::for_json_read() }
    #[doc(hidden)] pub fn __cpp_json_list<T: std::any::Any + Send + Sync>(
        &mut self, view: u64, owner: &str, id: u64, relation: &str, list: teaql_core::SmartList<T>) {
        self.install_json_relation_list(view, owner, id, relation, list)
    }
    #[doc(hidden)] pub fn __cpp_json_option<T: teaql_core::IdentifiableEntity + std::any::Any + Send + Sync>(
        &mut self, view: u64, owner: &str, id: u64, relation: &str, value: Option<T>) {
        self.install_typed_relation_option(view, owner, id, relation, value)
    }
}
impl crate::EntityRuntimeState {
    #[doc(hidden)] pub fn __cpp_json_view(self, view: u64) -> Self { self.with_json_view(view) }
    #[doc(hidden)] pub fn __cpp_json_view_id(&self) -> u64 { self.json_view_id() }
}
"""
JSON_GRAPH_DECODER_BRIDGE = """
impl crate::InMemoryEntityGraphDecoderRegistry {
    #[doc(hidden)] pub fn __cpp_json_capability(&self, descriptor: &teaql_core::EntityDescriptor)
        -> Result<bool, teaql_core::EntityError> { self.json_capability(descriptor) }
    #[doc(hidden)] pub fn __cpp_decode_json_edge(&self, entity: &str, rows: Vec<(u64, teaql_core::CompactRow)>,
        root: &crate::EntityRuntimeState, graph: &mut crate::EntityGraphBuilder,
        owner: &str, id: u64, relation: &str, many: bool) -> Result<(), teaql_core::EntityError> {
        self.decode_json_edge(entity, rows, root, graph, crate::registry::JsonEdgeRequest{owner,id,relation,many})
    }
}
impl crate::EntityRuntimeState {
    #[doc(hidden)] pub fn __cpp_weak_json_view(source: &Self, view: u64) -> Self {
        Self::fresh_with_weak_graph(source).with_json_view(view)
    }
}
"""
APPENDS = {
    "teaql-runtime/src/entity_runtime.rs": JSON_GRAPH_VIEWS_BRIDGE,
    "teaql-runtime/src/context.rs": CONTEXT_BRIDGE,
    "teaql-runtime/src/entity_save.rs": SAVE_BRIDGE + PREPARATION_BRIDGE + SAVER_BRIDGE,
    "teaql-runtime/src/data_service/mod.rs": LEDGER_BRIDGE + GRAPH_BRIDGE +
        LEDGER_EXECUTION_BRIDGE + MUTATION_BRIDGE + "\npub use graph::{" + NODE_EXPORTS + "};\n",
    "teaql-runtime/src/data_service/graph.rs": NODE_BRIDGE,
    "teaql-runtime/src/data_service/resolved.rs": ID_SET_PAGE_BRIDGE + LOADED_ROW_GRAPH_BRIDGE,
    "teaql-runtime/src/lib.rs": EXPORTS + JSON_GRAPH_DECODER_BRIDGE,
    "teaql-runtime/src/data_service/types.rs": """
impl<E> EntityDataService<'_, E> {
    #[doc(hidden)]
    pub fn __cpp_request_intent(&self, query: &SelectQuery)
        -> Result<teaql_core::QueryIntent, crate::RuntimeError> {
        self.request_intent_for(query)
    }
}
""",
    "teaql-runtime/src/data_service/relation.rs": RELATION_PLANNING_BRIDGE,
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def registry_packages(path):
    packages = []
    for section in path.read_text().split("[[package]]")[1:]:
        fields = dict(re.findall(r'^(name|version|source|checksum) = "([^"]+)"$',
                                 section, re.M))
        if fields.get("source", "").startswith("registry+"):
            packages.append(tuple(fields[key] for key in
                                  ("name", "version", "source", "checksum")))
    return set(packages)


def query_fragments(target_dir):
    """Verify the compiled fixture fragments contain byte-identical original methods."""
    fragments = {}
    for source, output, markers, wrapper in [
        ("data_service/base.rs", "runtime_base_query.rs",
         [("    pub(crate) fn new(", "    pub(crate) async fn insert(")],
         "impl<'a,M,E> RuntimeDataService<'a,M,E> where M: MetadataStore,E: "
         "teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Sync"),
        ("data_service/context.rs", "runtime_context_query.rs",
         [("    async fn observe<T, F, N>(", "    pub(crate) async fn insert("),
          ("    pub(crate) fn resolve_final_comment(", "\n}\n")],
         "impl<'a,E> ContextDataService<'a,E> where E: teaql_data_service::QueryExecutor + "
         "teaql_data_service::MutationExecutor + Send + Sync"),
    ]:
        original = (SOURCE / "teaql-runtime/src" / source).read_text()
        bodies = []
        for start, end in markers:
            first = original.index(start)
            last = original.index(end, first)
            bodies.append(original[first:last])
        expected = wrapper + " {\n" + "\n".join(bodies) + "\n}"
        candidates = list((target_dir / "debug/build").glob(
            "teaql_cpp_private_save_oracle-*/out/" + output))
        matching = [path for path in candidates if path.read_text() == expected]
        if not matching:
            raise RuntimeError(f"Compiled query fragment differs from original methods: {output}")
        fragments[output] = dict(source="teaql-runtime/src/" + source,
                                 markers=markers,
                                 method_sha256=[digest(body.encode()) for body in bodies],
                                 compiled_sha256=digest(expected.encode()))
    native_parts = []
    for source, markers in [
        ("data_service/relation.rs", [
            ("#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]",
             "impl<'a, E> EntityDataService<'a, E>"),
        ]),
        ("data_service/helpers.rs", [
            ("pub(super) fn relation_bucket_key(", "pub(super) fn aggregation_cache_namespace("),
            ("pub(super) fn graph_identity_key(", "pub(super) fn ensure_relation_target<"),
            ("pub(super) fn ensure_projection(", "pub(super) fn attach_empty_relation_aggregate("),
        ]),
        ("data_service/relation.rs", [
            ("    fn query_for_plan(", "    async fn fetch_relation_rows("),
            ("    async fn fetch_native_relation_rows(", "    async fn relation_facets("),
            ("    fn base_relation_query(", "    fn ensure_stable_top_n_order("),
            ("    fn ensure_stable_top_n_order(", "    fn query_for_compact_plan("),
            ("    fn query_for_compact_plan(", "    fn attach_relation_rows("),
        ]),
    ]:
        original = (SOURCE / "teaql-runtime/src" / source).read_text()
        for start, end in markers:
            first = original.index(start)
            last = original.index(end, first)
            body = original[first:last]
            native_parts.append(dict(source="teaql-runtime/src/" + source,
                                     markers=[start, end], body=body,
                                     sha256=digest(body.encode())))
    expected = "".join(part["body"] for part in native_parts[:4])
    expected += ("impl<'a,E> EntityDataService<'a,E> where E: "
                 "teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + "
                 "Send + Sync {\n")
    expected += "".join(part["body"] for part in native_parts[4:]) + "}\n"
    candidates = (target_dir / "debug/build").glob(
        "teaql_cpp_private_save_oracle-*/out/relation_execution.rs")
    if not any(path.read_text() == expected for path in candidates):
        raise RuntimeError("Compiled native relation fragment differs from original methods")
    fragments["relation_execution.rs"] = dict(
        compiled_sha256=digest(expected.encode()),
        parts=[{key: value for key, value in part.items() if key != "body"}
               for part in native_parts])
    enhancement_parts = []
    for source, markers in [
        ("data_service/helpers.rs", [
            ("pub(super) fn default_aggregate_value(", "pub(super) fn aggregation_cache_namespace("),
            ("pub(super) fn graph_identity_key(", "pub(super) fn ensure_relation_target<"),
            ("pub(super) fn ensure_projection(", "#[cfg(test)]\nmod aggregation_cache_key_tests"),
            ("pub(super) fn attach_relation_aggregate_rows(", "pub(super) fn ensure_initial_version("),
        ]),
        ("data_service/relation.rs", [
            ("    pub(crate) fn relation_aggregate_key_rows(", "    fn build_relation_plans("),
            ("    fn build_relation_plans_from_loads(", "    fn enhance_plan<'b>("),
            ("    fn relation_child_repo(", "    fn query_for_plan("),
        ]),
    ]:
        original = (SOURCE / "teaql-runtime/src" / source).read_text()
        for start, end in markers:
            first = original.index(start)
            last = original.index(end, first)
            body = original[first:last]
            enhancement_parts.append(dict(source="teaql-runtime/src/" + source,
                                          markers=[start, end], body=body,
                                          sha256=digest(body.encode())))
    expected = "".join(part["body"] for part in enhancement_parts[:4])
    expected += ("impl<'a,E> EntityDataService<'a,E> where E: "
                 "teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + "
                 "Send + Sync {\n")
    expected += "".join(part["body"] for part in enhancement_parts[4:]) + "}\n"
    candidates = (target_dir / "debug/build").glob(
        "teaql_cpp_private_save_oracle-*/out/relation_enhancement.rs")
    if not any(path.read_text() == expected for path in candidates):
        raise RuntimeError("Compiled enhancement fragment differs from original methods")
    fragments["relation_enhancement.rs"] = dict(
        compiled_sha256=digest(expected.encode()),
        parts=[{key: value for key, value in part.items() if key != "body"}
               for part in enhancement_parts])
    return fragments


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target-dir", type=Path,
                        default=Path("/tmp/teaql_cpp_private_save_rust_506"))
    parser.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    if lock["version"] != "5.0.6":
        raise RuntimeError("This observer is reviewed for Rust 5.0.6 only")
    archive = subprocess.check_output([
        "git", "-C", str(ROOT / ".reference/teaql-rs"), "archive", lock["commit"]])
    files = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            destination = SOURCE / member.name
            if not destination.resolve().is_relative_to(SOURCE.resolve()):
                raise RuntimeError("Archive path escapes the derived source tree")
            if member.isdir():
                destination.mkdir(parents=True, exist_ok=True)
            elif member.isfile():
                original = stream.extractfile(member).read()
                appended = APPENDS.get(member.name, "").encode()
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(original + appended)
                if not appended:
                    os.utime(destination, (member.mtime, member.mtime))
                actual = destination.read_bytes()
                if actual[:len(original)] != original or actual[len(original):] != appended:
                    raise RuntimeError(f"Original method body changed: {member.name}")
                files[member.name] = dict(original_sha256=digest(original),
                                          compiled_sha256=digest(actual),
                                          appended_bytes=len(appended))
    if not APPENDS.keys() <= files.keys():
        raise RuntimeError("An access bridge has no original source target")
    BUILD.mkdir(exist_ok=True)
    provenance = dict(
        schema=1, runtime_commit=lock["commit"],
        notice="Original archive bytes remain intact; only the listed test access bridges "
               "are appended. This is source-build provenance, not semantic acceptance.",
        appended_bridges=APPENDS, files=files,
        observers={family: digest((PROJECT / "src/bin" /
                                   (family + "_5_0_6.rs")).read_bytes())
                   for family in FAMILIES},
        build_status="prepared",
    )
    report = BUILD / "provenance.json"
    report.write_text(json.dumps(provenance, indent=2) + "\n")
    if args.prepare_only:
        print(f"Verified original bytes of {len(files)} archive files; access append only")
        return
    original_packages = registry_packages(SOURCE / "Cargo.lock")
    observed_packages = registry_packages(PROJECT / "Cargo.lock")
    if observed_packages - original_packages:
        raise RuntimeError("Observer registry dependencies differ from the original lock")
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.target_dir.resolve()),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER="/usr/bin/cc")
    command = ["cargo", "build", "--locked", "--offline", "--manifest-path",
               str(PROJECT / "Cargo.toml")]
    for family in FAMILIES:
        command += ["--bin", family + "_5_0_6"]
    subprocess.run(command, env=env, check=True)
    provenance["build_status"] = "completed"
    provenance["query_fragments"] = query_fragments(args.target_dir.resolve())
    provenance["observer_build_script_sha256"] = digest((PROJECT / "build.rs").read_bytes())
    provenance["cargo_lock_sha256"] = digest((PROJECT / "Cargo.lock").read_bytes())
    provenance["original_registry_versions_preserved"] = True
    provenance["binaries"] = {
        family: digest((args.target_dir / "debug" / (family + "_5_0_6")).read_bytes())
        for family in FAMILIES
    }
    report.write_text(json.dumps(provenance, indent=2) + "\n")
    print(f"Built {len(FAMILIES)} original private save observers; provenance: {report}")


if __name__ == "__main__":
    main()
