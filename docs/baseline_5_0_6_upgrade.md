# Rust 5.0.6 baseline upgrade

Status: references locked; behavioral migration and qualification are active.
Authorized by the user on 2026-10-11 while generator parity work was active.
This steering supersedes the initial goal's 5.0.5 runtime and earlier generator
reference without replacing its alignment objective or discarding ongoing edits.

## Exact references

- Runtime tag: v5.0.6, commit `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
  The remote annotated tag and local peeled commit agree. The workspace package
  version is 5.0.6. The clean detached reference is .reference/teaql-rs.
- Generator source reference: `45e1572c4956ca963410d8ea436ead3c8857087a`,
  recorded in that release's retained minimal generated fixture PROVENANCE.json.
  Inspect the clean detached .reference/teaql_code_gen worktree. The independent
  cpp_support implementation workspace remains based on `77b301e9`; changing a
  reference does not rewrite its ancestry or discard its C++ implementation.
- Previous runtime source is retained at .reference/teaql_rs_5_0_5. The historical
  lock is docs/parity/baselines/v5_0_5.lock.json. Original user checkouts remain
  untouched. The independent generator's existing uncommitted work is preserved.

## Behavioral delta and migration responsibilities

The release changes 328 repository paths. Fifty-eight changed Rust source files
belong to the five in-scope core/data-service/SQL/runtime/SQLite crates. The
[machine-readable delta](parity/baseline_5_0_6_delta.json) records old/new hashes
and pending dispositions. Counts and unchanged declarations are not acceptance.

| Contract family | Required review and qualification |
| --- | --- |
| Shared loaded state | Fixed property indexes, inline/overflow availability, immutable shared snapshots, sparse versus complete views, zero/false/Null/NotLoaded, view-local copy-on-write and independent mutations. Adapt sharing to explicit C++ Context lifetimes. |
| Typed checker reconstruction | Preserve projection indexes, snapshots, dynamic metadata and sparse-update rejection; Context/time fixes and their evidence must precede governed writes. |
| Request-owned intent and trace | Preserve root intent, canonical nested routes, branch/ledger attribution, late allocated IDs, SQL diagnostics and audit privacy. Invalid intent/provenance must reject before protected I/O according to source. |
| Dynamic fields and properties | Separate durable storage extensions from readonly derived properties; use trusted storage/profile provenance, Context metadata, explicit NULL/delete and transaction-backed SQLite persistence. |
| Hydration, JSON and streams | Shared projection layouts, alias validation, isolated same-identity views, bounded cursor loading, early close/lease cleanup and governed presentation-versus-storage authority. |
| SQL relations, aggregation and transactions | Membership, grouped partition/HAVING rebinding, forward filtered views, nested facets, logical trace paths and transaction lease isolation. |

Use the release's retained generated narrow fixture and shared-load-state/
trace-chain examples as additional fixed reference evidence. They supplement
source review and do not replace native C++ behavior comparisons.

## Evidence transition

Existing source inventories, module acceptance ledgers, committed oracle corpora,
measurement reports and earlier stage/Q/E checks were produced against 5.0.5.
Keep their provenance intact. Do not change their commit labels to manufacture
5.0.6 evidence. Baseline-bound comparison scripts correctly reject historical
corpora against the new active lock until the original calls are replayed and
requalified. Rust oracle dependency lockfiles may require a reviewed upgrade.

The active generator capability ledger is rebuilt from the new frozen producer.
Core source-map/disposition and module-root evidence refresh is part of P0/P7;
new or changed contracts stay pending until qualified. The historical six-stage
acceptance remains historical. Neither changing upstream.lock.json nor generating
fresh C++ headers constitutes completed 5.0.6 migration.

Continue under [cpp_generator_parity_plan.md](cpp_generator_parity_plan.md):
first dependency/source audit, then runtime contracts, then thin generated adapters
and actual application/source comparisons. SQLite-only scope, Context ownership,
camelCase application naming and targeted verification policy remain unchanged.

## Active source inventory and pending map

The separate [5.0.6 source inventory](parity/v5_0_6/upstream_inventory.json)
contains 124 files, 378 source declarations and 693 source tests.
[source_dispositions.json](parity/v5_0_6/source_dispositions.json) maps every
file to its existing or proposed package responsibility, retaining previous
status and unchanged/changed/new hashes separately from pending qualification.
All new-baseline dispositions remain pending semantic review. Regenerate and
check the inventory with:

```sh
python3 scripts/inventory_upstream.py --inventory-only --output-dir docs/parity/v5_0_6
python3 scripts/inventory_upstream.py --inventory-only --output-dir docs/parity/v5_0_6 --check
python3 scripts/verify_generator_parity.py
```

The generator verifier checks the release's producer provenance, frozen source
hashes/anchors and capability record consistency. Its successful normal run
is an integrity check. `--require-complete` requires qualified capabilities and
currently rejects all 17 pending records. It does not infer semantic coverage.

## Current paths and historical move evidence

workspace_paths_5_0_6.json now records active source/structure authority separately
from the immutable historical move and retirement ledgers. The default path
verifier checks seven package/target/module names, current source placement and
underscore spelling, all 124 fixed source/disposition links and 769 retained
historical destinations. It verifies the pinned source and fresh inventory; pending
semantic dispositions remain pending. Fifteen positive/negative controls exercise
baseline/hash/coverage/owner/name/path rejection without changing workspace files.

--historical-checkpoint also archives C++ 0.0.3 at
42e688ee59ea9a7671a0d0ea6a6549fd850e993f and runs that snapshot's original verifier:
769 retained files, 309 unchanged canonical production bodies, one relocation and
seven reviewed retirements pass. Current historical ledger bytes must equal the
committed checkpoint. This supersedes earlier reports that the old default check
rejects the upgraded lock, while preserving the underlying 5.0.5 evidence intact.
No active body digest or parity status is manufactured from current implementation.


## Durable dynamic core checkpoint

Distinct definitions/selection/value/mutation carriers now supplement readonly
properties. 296 original/C++ observations match frozen dynamic_fields.rs, with
native 3/3, targeted sanitizer 2/2 and module import 1/1 passing. The core source
map retains pending qualification and links this limited evidence. Runtime
entity/load-state/ledger integration and SQLite/JSON/stream storage remain open.
See [the core contract](dynamic_fields_5_0_6.md); this is not full G16 acceptance.


The subsequent loaded-state/row checkpoint records 176 direct snapshot/read-row
observations, distinct from the earlier 296 value-carrier cases. Native regression
passes 10/10 and module import 1/1. Read metadata now carries durable values and
readonly schemas separately from persistence columns; typed runtime entity and
storage integration remain pending. Historical 5.0.5 observations are unchanged.


Runtime dynamic state/ledger checkpoint records 147 unchanged public original/C++
observations and native 6/6, targeted sanitizer 4/4 and module import 1/1 gates.
Definitions/originals remain separate from native changes; composition and view
copying preserve their source responsibilities. Dynamic privacy joins governed
collection. Pending extensions reject before reads/writes until bound storage
exists, with a typed DynamicField runtime error. G16/G17 full qualification remains
open; see [the ledger contract](dynamic_ledger_5_0_6.md).


The data-service storage SPI checkpoint records 49 unchanged pure plan/write Debug
observations. Native 5/5, targeted sanitizer 4/4 and module import 1/1 pass. Executor
fixture query entry now supplies explicit comment/purpose while preserving its
existing delegation and transaction assertions. Source evidence is not relabeled
from 5.0.5. Actual SQLite/provider/cursor/transaction execution remains pending;
see [the SPI contract](dynamic_storage_spi_5_0_6.md).


Concrete SQLite storage checkpoint compares 123 real original/C++ store calls;
native 6/6, targeted sanitizer 5/5 and module import 1/1 pass. Transaction ownership
excludes ordinary work until terminal completion; failed consumed commit rolls
back immediately as source Drop does. Selected 5.0.5 fixtures gain mandatory
request intent and their changed isolation/cleanup assertions are requalified
natively without relabeling old corpora. See [the store contract](sqlite_dynamic_storage_5_0_6.md).
Runtime provider/graph/audit/readback and cursor enhancement remain open.


Context dynamic provider checkpoint adds the database/immutable fixture provider
SPI and UserContext resources, trusted stream/write preparation, consuming batch
conversion and source-shaped private audit values. The 72 direct frozen-source
observations use real SQLite storage and a registered SQL executor. Scope and
reproduction are recorded in
[dynamic_provider_5_0_6.md](dynamic_provider_5_0_6.md).
Actual query/graph hydration, transaction write/readback/audit integration and
thin generated adapters remain open; the ledger pre-I/O guard stays in place.
G16 and full 5.0.6 acceptance remain unqualified.


Dynamic query checkpoint adds scoped query selection, typed root/enhanced loading,
metadata-preserving decoding and real SQLite enhancement inside the cursor lease.
The 250 direct original/C++ cases match values, load bits, sharing and failure
contracts; see [dynamic_query_hydration_5_0_6.md](dynamic_query_hydration_5_0_6.md).
The handwritten carrier fixture does not establish generated School parity.
Relation/child fetching and embedded traversal, graph write/readback/audit,
complete hooks/trace/config/diagnostics and thin generated adapters remain open.
The ledger pre-I/O guard and all pending qualification labels are retained.


Dynamic relation checkpoint adds Context-driven relation/child loading, checked
child overlays, whole-batch embedded traversal and fetch-time attachment keys.
The separate 138-case original/C++ corpus preserves errors and original metadata
retention behavior; see [dynamic_relation_hydration_5_0_6.md](dynamic_relation_hydration_5_0_6.md).
Typed compact-flat graph hydration has native evidence. Source direct embedded
dynamic-only map retention, facets, complete typed/generated configurations,
graph write/readback/audit and full hook/trace/config/diagnostics remain open.
Historical 5.0.5 records are unchanged; G16 and complete 5.0.6 parity stay pending.


Transaction-local commit audit checkpoint implements operation-owned queues,
poll-only owner-thread activation, post-commit delivery and first-error wrapping.
Automatic ledger and manual scope tests preserve database commit status when a
sink fails; rollback, failed commit and cancellation discard facts. Native 9/9,
targeted sanitizer 6/6 (leak scanning disabled), module import 1/1, original private
unit tests 5/5 and 20 direct public error observations pass. See
[commit_audit_5_0_6.md](commit_audit_5_0_6.md) for scope and reproduction.
Dynamic native-fact enrichment is present, but the complete dynamic graph write/
readback path and generated adapters remain open. The ledger guard stays in place;
G16/G17 and all unqualified capabilities keep their pending status.


Dynamic ledger persistence checkpoint replaces the temporary blanket rejection
with transaction-backed preflight, native optimistic updates, extension apply/
authoritative readback and queued native-fact enrichment. Automatic and manual
save tests cover failure/rollback, NULL/delete, trigger values, assigned IDs,
foreign-key rebinding and explicit Global retention after UserContext deletion.
Fourteen direct public graph protocol observations match; the original SQLite
dynamic integration suite passes 23/23 separately. See
[dynamic_graph_save_5_0_6.md](dynamic_graph_save_5_0_6.md) for the exact evidence
and ownership adaptations. Complete non-root Checker/Fix/governance and all
hook/trace/privacy/configuration/generated-carrier matrices remain open.
G16/G17 and phases P0-P7 remain unqualified.

Final dynamic ledger gates: native 7/7, targeted sanitizer 7/7 (leak scanning
disabled), module import 1/1, formatting 798 files, 1,404 exports and seven
facade archives pass. All 17 generator capabilities remain unqualified.


Pending dynamic preflight checkpoint discovers native/dynamic/new/deleted related
keys using original values and normalized identities. Unrepresented dynamic keys
run the shared Checker/Fix pass before policy review; Fix output rejoins the
original ledger. Policy plans include extension-only operations and #code intent
values. Twenty-four direct public original/C++ denying-save observations match;
the separate original entity_save library selection passes 17/17. See
[pending_dynamic_preflight_5_0_6.md](pending_dynamic_preflight_5_0_6.md).
Generated Checker/Fix, readonly authority and complete hook/trace/privacy/config
matrices remain pending; no capability or complete phase is newly qualified.

Final pending-dynamic gates pass: native 7/7, targeted sanitizer 7/7 (leak
scanning disabled), module import 1/1, 800 formatted files, 1,404 exports and
seven facade archives. All 17 generator capabilities remain unqualified.


Generated dynamic-carrier checkpoint adds the shared GeneratedDynamicFields
helper, thin indexed Model install/mutation adapters, camelCase application
methods and ref-qualified Q selectDynamicFields. All five fixtures regenerate
and reproduce (55 files), selected Java tests pass 11/11, and 80 direct frozen
entity-macro observations match. Native 5/5, targeted sanitizer 5/5 (leak scanning
disabled), modules 3/3, formatting 804 files, 1,406 exports and seven facade
archives pass. See [generated_dynamic_fields_5_0_6.md](generated_dynamic_fields_5_0_6.md).
The original generator checkout remains untouched. Full readonly, generated
configuration and remaining G01-G17/P0-P7 acceptance stay open.
