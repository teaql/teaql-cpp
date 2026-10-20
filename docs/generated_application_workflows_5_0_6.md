# Progressive generated application workflows

This G12 checkpoint supplies model-specific, executable application examples for
the seven locked query/mutation/expression actions. It also repairs actual
generation failures for legitimate namespaces. Full assembly, sample-data,
remaining Assist and G13/P7 responsibilities remain active; G12 stays partial.

References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Discovery and source responsibilities

Start with the entity heading in generated `ASSIST.md`, follow its action link,
then consult that model's field entries for the exact writable operations.
`AGENTS.md` documents this progressive path. Files live at
`assist/<cpp_entity_identifier>/<action>.md`; canonical KSML names remain in the
documents. Reserved words use the established C++ identifier escape in directory
names. Plurals, predicates, E entries and getter names come from the same shared
metadata/naming helpers as the SDK.

Each document contains one complete C++ block. These are application function
definitions to copy into an application, not additional methods in the generated
SDK. The generator emits no new runtime query, persistence or ownership algorithm.

| Action | Source responsibility retained by the C++ example |
| --- | --- |
| `create` | Create through the bound Q entry, initialize explicitly allowed typed fields, attach an audit reason and use the existing Save runtime. |
| `query` | Use the actual minimal entry and human/non-human ID predicate, stable ID ordering, a bounded result and nonblank intent. |
| `update` | Load complete scalar state and the authoritative original version before editing; return empty for a missing row and perform an audited Save. |
| `delete` | Load the current entity, mark its generated deletion intent and save with audit; missing rows return false. Versioned SQLite deletion remains soft. |
| `list_page` | Validate example endpoint bounds, retain a stable unique order and execute the typed page/count contract without accepting a hard-limit override. |
| `stream` | Use an ID-only projection, fixed bound of 100 and chunk size 32; check each item Result before reading IDs into a managed Context vector. |
| `expression` | Emit actual E evaluations for every scalar/FK, forward relation and reverse relation getter; preserve loaded NULL and NotLoaded without implicit SQL. |

Create/update callbacks receive the typed model; they are where application code
selects authorized writable fields. They do not mass-assign input JSON. Trusted
Context assembly still supplies authorization and tenancy. Modeled roots and
constants retain their governed bootstrap path rather than being reseeded by an
ordinary operation.

## Explicit C++ adaptations

Runtime query results are read-only borrows. Update/delete make an explicit
managed editable copy in the same `user_context`, preserving loaded state and
the original optimistic version. This is distinct from client JSON decoding,
cross-domain retention and composing another entity's pending mutation graph.

Update returns an optional reference to the native saved model; create/query/
page/stream results likewise borrow their Context. Ordinary wrappers do not
extend lifetime. Destroy them before close or copy explicitly into a destination
scope. Delivered stream allocations remain until Context close; the example
does not claim fixed-memory unbounded streaming.

The snippets use the existing generated `require_value` convenience to report
Result errors as exceptions. Applications needing typed error inspection should
check the original Result before unwrapping. An audit-delivery error after commit
must not be retried as a rolled-back business transaction.

## Reproduced namespace defect

The Assist naming pass built `Set.of(namespace, fixed_protocol_names...)`.
Namespaces such as `from_json` and `user_context` are valid, but duplicate set
entries caused generation to throw. The
[original observations](parity/assist_workflow_namespace_before_5_0_6.json.gz)
retain both actual failures and a successful ordinary control.

The pass now deduplicates with `HashSet` while retaining exactly the same token
membership behavior. It uses membership only, so set iteration cannot change
output ordering. Three repaired namespaces are separately compiled with actual
model SDKs and all their action documents; see the
[namespace compiler record](parity/assist_workflow_namespace_compiler_5_0_6.json.gz).

## Generation and execution evidence

The [pre-change snapshot](parity/assist_workflow_before_5_0_6.json.gz) preserves
the producer, ledger and every file in all 77 SDK destinations. The
[Java record](parity/assist_workflow_java_5_0_6.json.gz) retains the intended
duplicate-name regression and **73/73** successful tests, including authentic
human naming, irregular/uncountable plurals, reserved words and action discovery.

Real Java generation emits **1,253** action documents for **179** model entries
in **63** successful SDKs. All **77** destinations strictly reproduce: 24 ordinary
workspaces, 50 contract destinations and three identity fixtures. Every existing
C++ code byte and catalog-data byte is unchanged. Only action documents, index/
agent guidance, fingerprints and exact generated-file counts account for the
changes. The [regeneration proof](parity/assist_workflow_regeneration_5_0_6.json.gz)
retains full file sets and command results; rejected fixtures preserve their
actual errors.

The [compiler gate](../scripts/verify_assist_workflow_compilation.py) extracts
the actual C++ fences and compiles each SDK independently, explicitly
instantiating both callback templates for every model. Shared production headers
alone use a PCH; incompatible model variants never share an SDK PCH. On failure,
an independent SDK-only control distinguishes old SDK problems from an invalid
action example. Results are retained in the
[compiler record](parity/assist_workflow_compiler_5_0_6.json.gz).
All **63/63** SDK sets compile successfully, including **358** explicit callback
template instantiations. The three namespace controls compile another **105**
complete action documents. Native and imported-module execution each pass
**1/1**; pinned formatting and its check pass for **1,643** handwritten files.

The [complete producer edit proof](parity/assist_workflow_producer_scope_5_0_6.json.gz)
restores every previous producer byte after removing only the declared new
documentation recipes/index/rule and reversing duplicate-safe set construction.
Existing implementation bodies are preserved alongside the whole-SDK proof.

Two unsuccessful PCH preparations are preserved separately: the
[missing compile-only flag](parity/assist_workflow_compiler_preflight_5_0_6.json.gz)
and [incorrect assumed umbrella name](parity/assist_workflow_compiler_header_preflight_5_0_6.json.gz).
Neither ran an SDK/action check or counts as successful compilation.

The [native integration test](../tests/generator/assist_workflow_tests.cpp)
executes the extracted examples against actual SQLite. It verifies create/read,
loaded versus missing scalar reads, zero implicit E queries, full-state update,
missing update, stale rejection, sparse relation-model rejection, full relation
update, stable pages/count, invalid pages before I/O, a 100-row stream crossing
chunks, soft deletion and the retained negative version. The same source and
extracted header run through the named-module frontend.

Five independent compiler negatives preserve missing purpose/audit, unknown
fields, extra runtime arguments and Context parameters on Q entries. The
[terminal record](parity/assist_workflow_terminal_5_0_6.json.gz) joins current
source, native/module, compiler, reproduction and formatting evidence. No new
sanitizer, MIPS or physical-device result is implied.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_assist_workflow_tests
ctest --preset native --output-on-failure -R '^assist_workflow_contracts$'
cmake --build --preset modules_native --target teaql_module_assist_workflow_tests
ctest --preset modules_native --output-on-failure -R '^module_assist_workflow_contracts$'
```
