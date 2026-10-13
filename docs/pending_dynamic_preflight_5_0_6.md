# Pending dynamic entity preflight and governance

Recorded: 2026-10-12. Baseline:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. This extends
[dynamic ledger persistence](dynamic_graph_save_5_0_6.md); it does not qualify
all generator phases or complete G16/G17.

## Source responsibilities

Rust `entity_save.rs::hydrate_ledger_relations` discovers pending entities from
native changes, dynamic changes, new keys and deleted keys. It reconstructs each
candidate from original values plus current native changes. Relation comparison
uses EntityKey identity normalization, allowing signed SQLite IDs to match unsigned
generated references. An untyped NULL local reference does not match a NULL foreign
reference. Discovered children retain comment, original values and dirty fields;
removed children do not recursively discover descendants.

Represented children run the existing graph Checker/Fix pass at their model path.
`preflight_pending_dynamic_entities` separately reconstructs dynamic keys absent
from the graph, skipping represented or deleted keys. It supplies native loaded
fields, dirty fields and create/update status to the same Checker/Fix contract,
using the source's root location. Fix output is merged back into the original
ledger. Automatic and explicit-executor audited save call this pass after graph
preflight/composition and before policy review or protected execution.

Rust `mutation_policy.rs::ledger_policy_plan` includes dynamic-only keys in its
ordered operation set and adds `#code` SET/delete operation values to changed_values.
Native and dynamic operands share the same policy operation, original version and
request reason. A denying policy must see extension-only operations and stop the
save before native or extension writes. New-and-deleted cancellation semantics
remain unchanged.

These mechanisms remain independent from SQL execution, dynamic storage validation,
transaction-local business facts and safe logging. They add no dependency and do
not introduce a second Checker/Fix implementation or hidden ownership domain.

## Evidence and boundaries

The native preflight fixture checks related and unrepresented dynamic entities,
exactly-once Checker invocation, original signed FK matching, native Fix writeback,
root versus relation diagnostic paths, reconstructed loaded fields, rejection,
deleted keys and NULL relation exclusion. Real SQLite save tests verify that a
policy receives an extension-only operand/version and can reject it while native
and extension storage, audit facts and retryable pending intent remain unchanged.

Twenty-four direct original/C++ cases call the public typed ledger-save function
with a denying policy. They compare ordered root/non-root operations, canonical
request/root/reason, loaded versions, native changes, dynamic SET/NULL/typed-NULL/
delete values, exact denial text and retained pending intent. Unicode and long
operands remain fixture data. The explicit policy projection excludes volatile
execution IDs; no output normalization is applied. The original generated Rust
School carrier and the existing handwritten C++ carrier exercise the shared runtime;
this corpus is not C++ generated-carrier acceptance or successful-write qualification.

The unchanged original runtime `entity_save::` library selection passes 17/17
separately. That source suite supplements review and does not establish C++ coverage
of all its private tests. Complete generated Checker/Fix behavior, inheritance,
readonly/presentation authority, all hook/trace/privacy/configuration combinations
and generated adapters still need qualification. All 17 generator capabilities
retain their unqualified status.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
cmake --build --preset native --target teaql_dynamic_preflight_tests \
  teaql_dynamic_graph_save_tests teaql_dynamic_governance_probe \
  teaql_mutation_policy_tests teaql_audited_save_tests \
  teaql_graph_saver_tests teaql_transaction_scope_tests
ctest --test-dir build_native_current --output-on-failure \
  -R '^(dynamic_preflight_contracts|dynamic_governance_rust_parity|dynamic_graph_save_contracts|mutation_policy_context_contracts|audited_save_context_contracts|graph_saver_context_contracts|transaction_scope_context_contracts)$'
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_commit_audit_original \
cargo test --manifest-path .reference/teaql-rs/teaql-runtime/Cargo.toml \
  --offline --locked --lib entity_save::
```

Build the public original probe from `tests/parity/rust_field_layout/Cargo.toml`
with `--offline --locked --bin dynamic_governance_5_0_6`, the explicit host linker
above and `CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506`. Replay/regenerate
with `generate_dynamic_governance.py --rust <rust-probe> --cpp <cpp-probe>`.
Final gates pass: native 7/7, targeted ASan/UBSan 7/7 with leak scanning disabled
(`ASAN_OPTIONS=detect_leaks=0`), module import 1/1 and live public governance
observations 24/24. The separate original library selection passes 17/17.
Formatting checks 800 handwritten C++ files using pinned clang-format 18.1.8;
1,404 catalog exports and seven facade archives verify. Source/path checks cover
seven packages, 115 production units and 124 pending source dispositions.

Build the same seven targets with `--preset sanitize` and run the same selection
in `build_sanitize_current` using `ASAN_OPTIONS=detect_leaks=0`. Build
`teaql_module_contracts` with `--preset modules_native`, run `module_import_contracts`
in `build_modules_native`, and verify archive ownership with
`scripts/verify_module_symbols.py --build-dir build_modules_native`.
No MIPS or physical-device execution is claimed for this checkpoint.
