# Generated runtime support contract

This is the historical 5.0.5 contract and acceptance record. Current 5.0.6
purpose, facet provenance and private diagnostic-source behavior differ; see the
[exact-reference replay](generated_support_reference_5_0_6.md) and
[derived-query implementation](derived_query_diagnostics_5_0_6.md). Historical
results below do not establish current whole-source acceptance.
The subsequent [complete 5.0.6 review](generated_support_source_review_5_0_6.md)
qualifies GS01-GS19 with its own current and explicitly reused unchanged evidence.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-runtime/src/generated_support.rs`. This is a runtime module used by
handwritten or generated callers. Porting it does not modify the generator.

## Production source review

The complete 321-line module is reviewed: four query-service signatures and
associated Error, two typed entity-service signatures, their EntityDataService
delegations, error alias, two TeaqlRuntime methods, the hidden consuming-save
trait, both purpose wrappers and both facet functions. Streaming remains a
required query-service capability; hidden AuditedSave has no implementation in
this source module. Only PurposedSelectQuery derives Clone/Debug. Neither wrapper
derives Default, equality, hashing or serde; no such protocol is invented.
Trait delegation uses the existing entity service rather than a duplicate
runtime/executor layer. Its later complete orchestration has its own source gate.

## Public abstractions

| Original abstraction | C++ counterpart and responsibility |
| --- | --- |
| TeaqlQueryDataService | Structural concept preserving fetch-all, smart-list, relation-aggregate smart-list, and stream signatures with the associated executor error. A non-streaming entity service fails this concept. |
| TeaqlEntityDataService | Extends the query contract with both typed enhanced-entity methods. C++ checks it for an explicit model type; handwritten models satisfy the same Entity contract. |
| TeaqlDataServiceError | Associated DataServiceError alias, retaining runtime/entity/executor error separation. |
| TeaqlRuntime | Independent interface exposing UserContext and facet smart-list fetching, including query purpose, runtime relation aggregates and the separate trace chain. |
| AuditedSave | Hidden consuming-save contract with an associated error and GraphNode result. It remains distinct from AuditedSaveExt. The pinned module supplies no implementation; checking its shape does not invent a saver. |
| PurposedQuery<T> | Public inner/purpose fields. General purpose text may be empty or whitespace; construction does not require a Clone-capable inner. C++ allocator-aware moving supports move-only inputs. |
| PurposedSelectQuery | Private SelectQuery, nonempty Unicode-trimmed purpose validation, an untrimmed Purpose trace, readonly access and consuming query extraction. Explicit-domain copies do not append another purpose. |
| restrict_facet_to_outer_query | Metadata-based IN-subquery restriction, retaining the original error variants and selective field clearing. |
| execute_facets | Sequential facet orchestration through TeaqlRuntime, without merging executor, provider, request or runtime responsibilities. |

The corresponding aliases are available through `teaql/runtime.hpp`. C++ concepts
express the source service responsibilities through existing EntityDataService
methods; they do not introduce a second service implementation or erase streaming.

## Restriction and execution order

Restriction looks up the outer entity descriptor first. Absence produces
`RuntimeError::Graph("missing entity: ...")`, not MissingEntity. Only after that
lookup does it look up the named relation and produce MissingRelation if absent.
The copied outer query clears projection, expression projection, ordering,
pagination slice, aggregates, group fields and relation loads. Other fields,
including HAVING, hard limit, trace, raw SQL, search, caches, streaming and
pagination options remain present. Expr::in_subquery then selects the relation's
local key and filters the facet's foreign key. The source outer query stays
unchanged. The remaining QuerySelection fields and options are preserved.

For each requested facet, in source order:

1. Clone the complete selection and merge the outer filter into builder relation
   aggregates whose query entity exactly matches the outer entity.
2. Apply restriction unless include_all_facets is true. That flag does not skip
   the aggregate filter merge and does not require outer metadata lookup.
3. Materialize builder relation aggregates separately and apply request runtime
   metadata/child enhancements to the facet query. The helper does not apply
   the facet selection's relation_selections directly.
4. Copy the outer trace chain and append a Relation trace with the facet's entity
   and name. Separately construct a PurposedSelectQuery with the exact
   `Calculate facet ` prefix and untrimmed facet name.
5. Await TeaqlRuntime's facet result before starting the next facet. Stop at the
   first restriction or fetch error. Duplicate names replace earlier results in
   the returned ordered map; each duplicate still executes.

Complete SmartLists, including totals, aggregation/summary records and nested
facets, are retained. This helper does not reconstruct rows locally or silently
turn fetch errors into empty facets.

## C++ ownership

Facet operations, selections, purposed queries, aggregate queries and traces use
the invoking UserContext resource. The entity-name argument references the
operation-owned purposed query, so it remains valid across a suspended fetch.
Checked runtime borrowing rejects early destruction before virtual fetch
dispatch. Wake forwarding detaches and the pending child is cancelled/joined
before referenced state is reclaimed. A foreign-domain child future is rejected
without cancelling another Context's work.

Returned references are User borrows. Explicit Global copies propagate into rows,
nested values, summary/aggregation records and nested facets. Another User must
rebind Global access after actual deletion of the original User. There is no
shared owner, default PMR fallback or third ownership lifetime. Pure query/request
helpers retain explicit Memory inputs. Context close, polling and allocation
remain owner-thread operations.

The generic purpose wrapper consumes allocator-aware move-only inputs, and
allocator-aware moving rebinds both inner state and purpose characters. Custom
inner types remain responsible for correct allocator propagation and explicit
cross-domain movement/copying. The wrapper cannot inspect arbitrary pointer
graphs. General wrapper copies are a C++ ownership adaptation, not a newly
required Rust Clone bound.

## Purpose wrapper diagnostics follow-up

The pinned private-query wrapper derives Clone and Debug; the generic
PurposedQuery does not derive either. A public umbrella compile fixture
reproduces the missing PurposedSelectQuery diagnostic writer. The dedicated
purposed_query_debug.hpp now writes the original one-field structure and delegates
the complete nested SelectQuery with inherited formatter options. The same
umbrella compile fixture succeeds after the repair.

The existing original generated_support oracle adds an optional protocol
observation without changing the older 370 scripts. Forty-six new inputs agree
across two original Rust executions, covering nine formatting modes, cloned
wrapper diagnostics, readonly query access and consuming extraction. Inputs
include complete query fields, preexisting purpose traces, empty/Unicode-blank
purposes, nonblank invisible characters, quotes, control characters and long
strings. Cloning never adds another Purpose trace. The observation JSON is a
fixture envelope, not a new query serialization contract.

A native lifetime contract retains the private wrapper through UserContext's
Global proxy, deletes its source User, verifies every nested query/string
allocator, then clones and extracts into a new User. Default PMR remains disabled.
Fresh named targets finish before selected checks. ASan/UBSan passes 3/3 in
11.24 seconds with leak scanning disabled; MIPS/QEMU passes 3/3 in 14.26 seconds.
Both include the original 370 semantic scripts and the new 46 diagnostics, and
all 46 MIPS outputs also match live original Rust. The complete native refresh
finishes and reports no work; its full regression passes 318/318 in 133.86 seconds. The complete module is
accepted within the documented Context ownership and future-interface
adaptations. Later entity-service orchestration, source-test/root acceptance,
runtime privacy and stage completion retain independent gates.

## Evidence and remaining acceptance

The differential corpus contains 370 unchanged original public-call scripts,
including 24 real SQLite executions through EntityDataService, the SQL executor
and SQLite provider on both sides. It records complete SelectQuery Debug,
runtime aggregate queries, distinct trace chains, complete result lists, typed
errors, first-suspension snapshots and original future-drop cancellation.
Recording and SQLite adapters implement only the application-supplied TeaqlRuntime
SPI; they do not copy the restriction or execution algorithms.

Native contracts cover pending argument lifetimes, early runtime destruction,
foreign cancellation authority, actual User deletion after a deep Global copy,
move-only generic inputs, query/entity service concept instantiation with a
handwritten model and the distinct hidden audited-save shape. These establish
this module's selected API/lifetime contracts. Complete source-test acceptance,
full runtime privacy/log acceptance and the other migration stages remain open.

```sh
cmake --build build --target teaql_generated_support_tests teaql_generated_support_probe
ctest --test-dir build --output-on-failure -R '^generated_support_(contracts|rust_parity)$'
python3 tests/parity/compare_generated_support.py \
  --cpp build/teaql_generated_support_probe \
  --rust /home/philip/shared-cargo-target/debug/generated_support_oracle
```

The sanitizer and MIPS configurations use the same native contracts. The parity
CTest supports the configured CMake QEMU emulator. Physical-router execution is
a separate unverified gate; source-name checks and these scripts do not prove
all six migration stages complete.

Acceptance on 2026-10-07: after rebuilding all consumers, the full native Debug
regression suite passed 178/178. The generated-support contracts and 370-script
corpus passed 2/2 selections under ASan/UBSan with leak detection and 2/2 under
OpenWrt MIPS/QEMU. A separate live original Rust/MIPS comparison matched
370/370. All eight unchanged original core dynamic-search integration tests
passed; their C++ source-test port and separately launched privacy canary passed
native, sanitizer and MIPS selections. Pinned source, inventory, root reexport
names and clang-format 18.1.8 checks passed (432 handwritten C++ files).
