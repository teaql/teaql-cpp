# Forward relation owner identity (Rust 5.0.6)

Status: FRI01 is corrected and G01 is requalified. Expanded installed
4,800/4,800, native 10/imported modules 4, Java 89, twelve result controls,
three strict prefix controls and full 80-SDK restricted reproduction pass.
The exact previous producer reproduces 76 differences. Runtime implementation
and layout are unchanged. G13/G14/G17 and P0-P7 acceptance remain open.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

The original `entity/struct.rs.stg::objectRelationMethods` passes the inherent
`Self::ENTITY_NAME`, whose value is the model's raw declared name. The graph
canonicalizer makes declared `item` and generated `Item` equivalent keys.
An overridden descriptor `Other` is a separate key. Reverse handles explicitly
use the entity trait identity; the derived JSON projection also uses the trait
identity. Those three caller responsibilities must remain separate.

C++ forward getters currently pass their descriptor `ENTITY_NAME`. This can
select a different trusted graph view or fall back to the identity cache where
the original returns a different value or NotLoaded. Both-view cases make the
distinction visible: a getter can return model view 200 while JSON contains
trait view 300. Neither identity should be normalized into the other by the
comparison or by a runtime-wide ownership rewrite.

The [complete before run](parity/generated_relation_identity_full_before_5_0_6.json.gz)
extracts and verifies complete original SDKs from immutable retained compiler
evidence. It compiles against the locked runtime and registry graph, then
freshly generates, compiles and runs C++ SDKs from the same models. No original
method is patched. The matrix crosses Item/Child, named relation availability,
zero/nonzero foreign IDs, cache absence/presence, missing/raw/canonical/trait/both
view owners, present/empty/wrong-type views and an initial nested JSON projection.
Context JSON decoding constructs relation views and does not itself prove a
local embedded object. An expanded compact-row constructor will exercise the
separate actual embedded-object branch and ordinary graph-view identity.

Each outcome retains the actual getter ID, direct EvalResult, E result or all
nine diagnostic fields, complete JSON, three independent runtime edge lookups,
raw/trait names and SQL count. Only the established E API-spelling/English
presentation adapter applies. All SQL counts are zero; this is an explicit
trusted in-memory graph read, not implicit database loading or persistence.
Default PMR is disabled, actual UserContext deletion is checked and temporary
build trees are removed after child processes exit.

The [first observer attempt](parity/generated_relation_identity_initial_5_0_6.json.gz)
fails compiling its outer Rust function pointer; a closure corrects that fixture
call. The [second attempt](parity/generated_relation_identity_before_5_0_6.json.gz)
executes the original control but its C++ observer incorrectly calls the static
EvalResult factory as an accessor. The actual accessor is `get()`. Both failures
remain separate from the successful comparison and require no production fix.

## Runtime first and thin owner metadata

The existing graph implementation already normalizes declared/canonical aliases
and keeps a descriptor override in a separate owner namespace. A permanent
runtime contract now checks both namespaces, selected NULL versus missing,
wrong-type optional lookup, explicit Global graph copying, actual source Context
deletion and rejection of the escaped checked handle. The scoped
[kernel gate](parity/generated_relation_identity_kernel_5_0_6.json.gz) passes
native 1/1 and imported modules 1/1 before producer installation. No production
runtime method or layout changes.

The original forward template's `Self::ENTITY_NAME` is its raw model constant.
The producer now passes `od.getName()` as one literal at that existing lookup.
Its availability guard, embedded-object precedence, typed edge lookup and
identity-cache fallback retain their existing bodies. Reverse handles and JSON
continue to use the entity trait identity. This changes no application signature,
ownership domain, Context parameter, dependency or naming convention.

The [restricted SDK update](parity/generated_forward_identity_sdk_update_5_0_6.json.gz)
reproduces all 80 active SDKs. Exactly 145 forward lookup owner arguments in
53 model headers change. Each literal must match that model's already verified
raw identity metadata; replacing only those arguments and formatting restores
every previous byte. All other model/query/checker/E bodies and guidance remain
unchanged. All 2,467 protected original/historical artifacts remain unchanged.
The [post-format installed check](parity/generated_forward_identity_sdk_installed_5_0_6.json.gz)
verifies all 3,560 generated-file hashes. The current Java gate passes
[89/89 tests](parity/generated_forward_identity_java_5_0_6.json.gz), including a
permanent required/optional/inherited/multiword/keyword owner regression and
explicit preservation of reverse and JSON trait-identity calls.

## Expanded constructors and complete outcomes

Context JSON decoding creates indexed relation views. Runtime-state attachment
preserves the entity's loaded view identity; an initial nested JSON object is
not proof that a local boxed target exists. The expanded observer retains every
original JSON input and outcome, then adds the same matrix through public
`from_compact_row` construction. A physical object-valued forward slot constructs
an actual embedded Platform. This separately checks the source's first branch,
including when the named relation is unselected and graph/cache views disagree.

The [expanded installed run](parity/generated_forward_identity_expanded_installed_5_0_6.json.gz)
freshly compiles/runs complete original and current SDKs for **4,800/4,800**
matching outcomes. It preserves 1,652 values, 3,148 NotLoaded results, 1,200
actual embedded-target results and 28 differing getter/JSON target pairs.
All SQL counts remain zero. All 2,400 earlier original outcomes are an exact
checked prefix, including their JSON-view attachment behavior.

The [independent result controls](parity/generated_forward_identity_controls_5_0_6.json.gz)
check exact result responsibilities, raw/trait names, canonical diagnostic roots,
complete original/native terminal stdout and source/registry/SDK lineage.
Every embedded target must win before graph availability; distinct getter and
JSON views must remain represented. Twelve effective corruptions reject missing
SDK/read, boolean SQL count, implicit SQL, wrong getter owner, merged JSON owner,
dropped EvalResult/diagnostic fields, wrong root, altered original human text,
added fields and merged kernel namespaces. The original human message is checked
before the established English presentation adapter.

The [exact previous-producer control](parity/generated_forward_identity_previous_control_5_0_6.json.gz)
reconstructs its full Java file by reversing only the owner argument. Its hash
matches the actually executed before checkpoint. Five freshly generated previous
C++ SDKs reproduce **76 differences** against the immutable expanded original
results; current producer files remain untouched. Both earlier source and native
2,400-record prefixes remain exact. The subsequent
[strict prefix audit](parity/generated_forward_identity_previous_prefix_strict_5_0_6.json.gz)
uses the shared type-aware comparison and rejects boolean/integer substitutions
on either side plus a dropped SDK. This is read-only strengthening of retained
terminal evidence, not another claimed compiler execution.

## Final scoped gates

The [first integration controller](parity/generated_forward_identity_installed_gates_5_0_6.json.gz)
passes all ten native tests, then stops because it requests an unregistered
generic module test name. It does not execute a missing test or suppress a
failure. Discovery identifies the real imported runtime, generated identity,
identity-kernel and SQLite parity contracts. The
[repaired current gate](parity/generated_forward_identity_installed_gates_repaired_5_0_6.json.gz)
passes native **10/10** and imported modules **4/4** with unchanged input hashes.

This closes FRI01 and restores G01's preceding qualified scope. G13/G14/G17 and
full P0-P7 exits remain open. The runtime implementation is unchanged, so this
metadata/test correction does not rerun sanitizer, MIPS or the full suite. The
historical complete native 877 epoch remains historical after prior runtime
changes. All 1,808 handwritten C++ files pass pinned formatting and checking.

## Source and artifact preservation

The [bounded source/body review](parity/generated_forward_identity_source_review_5_0_6.json.gz)
checks all 38 initial differing fields and every independent kernel/JSON pair.
Graph runtime declarations/implementation and JSON runtime bytes still match
their Git checkpoint; the previously qualified Q/E/runtime changes are preserved.
The [cleanup receipt](parity/generated_forward_identity_cleanup_5_0_6.json.gz)
confirms eight exact task-owned directories are absent after terminal children.
Successful measured removals total 937640999 bytes; unmeasured failure/reproduction
paths are separately marked without inventing a size. Stable preset outputs and
shared Maven/Cargo downloads remain available. No unrelated cleanup or commit/push
is part of this checkpoint.
