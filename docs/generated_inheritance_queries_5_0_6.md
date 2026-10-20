# Generated inheritance query contracts: Rust 5.0.6

Current status: G04/P2 is qualified with documented language/ownership adaptations;
normal inheritance generation is enabled. See [the final acceptance](inheritance_acceptance_5_0_6.md).
The checkpoint status and guard statements below are retained historical evidence.

Status: P2 qualification in progress. Default generation retains the inheritance
guard. This checkpoint supplements the field-model evidence in
[generated_inheritance_5_0_6.md](generated_inheritance_5_0_6.md).

## Source-defined construction

The frozen generator's request/index.stg defines `match_types`, `with_type_group`,
`enhance_child`, `enhance_children_if_needed`, direct-child type filters and
the Q `*_with_children` factory. Runtime source remains v5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the producer remains
`45e1572c4956ca963410d8ea436ead3c8857087a`.

Type membership uses `internal_type`; a generated child filter includes that
child and all its descendants. `withTypeGroup()` selects `type_group`. Neither
operation creates a storage column or invents a backend inheritance scheme.

The with-children entry clears slicing, appends flattened self fields and
recursively adds direct children using their own with-children entries. It has
no default live-version predicate, including within descendants. Ordinary and
minimal entries retain that predicate. The hard limit remains independent of
the slice, as in the original generator.

Request construction starts with **own** identity/version fields. In this model,
School starts with id/version, while Academy and ResearchAcademy start with id
only: their version is inherited. Subsequent self selection includes all
predefined fields, preserving source projection order and duplicates. Generated
`QUERY_IDENTITY_FIELDS` records this model-specific metadata. Runtime owns the
construction algorithm; handwritten models without that metadata retain the
descriptor-based identity/version default.

## C++ ownership adaptation

The runtime `EntityQuery` facade implements `matchTypes`, `withTypeGroup`,
`enhanceChild`, `childQuery` and `unlimited`. Generated adapters emit child type
lists and assemble recursive selections after all request types are complete.
Each fluent method preserves lvalue/rvalue behavior; purpose still stages
execution, which receives exactly one UserContext.

Child enhancement snapshots the supplied query. It validates Context identity
and allocation domain before retaining data. Cross-Context or User-to-Global
composition requires an explicit destination copy. Recursive construction in a
retained Global query allocates the descendants in that Global domain through
UserContext. Ordinary request copies are independent Context-owned snapshots;
wrapper destruction does not reclaim or extend Context data.

The application contract tests cover helper-returned selections, later child
mutation, incompatible Context/domain rejection, nested destination allocation,
explicit Global retention/rebinding and unexecuted query access after actual
UserContext deletion. Default PMR allocation is disabled during those tests.

## Comparison boundary

The paired probe calls the original generated requests without modifying their
files. It observes entity identity, ordered projections, exact Expr and Slice
Debug, hard limits and recursively converted child selections. The fixture
matrix combines normal/minimal/with-children entries, direct-child filters,
arbitrary/empty type values, type-group selection, slicing/unlimited, repeated
recursive enhancement and explicit sparse child requests.

Full SelectQuery Debug initially exposed additional differences: C++ omits
source facets, relation-aggregate query members and readonly-definition metadata.
Those broader query families remain open under G08/G16/P5/P7. They are not
normalized away or qualified by the inheritance projection corpus. The selected
inheritance observations compare exactly within their stated scope.

The final construction matrix passes 1800/1800 unchanged original/C++
observations. Selected native contracts pass 9/9; sanitizer lifetime/integration
checks pass 3/3 with leak scanning disabled; module consumption passes 2/2.
Compiler contracts pass one positive and three negative cases, including a
non-direct subtype, missing purpose and an untyped child. Selected Java tests
pass 14/14; all seven generated sets reproduce byte-for-byte (77 files).

Return-type adaptation, typed polymorphic decoding, stream behavior, query
failure contracts and governed graph/save comparisons remain
required before G04 or P2 can be accepted or the normal generation guard removed.

## SQLite execution qualification

The actual original generated service runtime and requests now have a separate
SQLite probe. It compares raw compact-row execution and typed list serialization
for all three inheritance levels, matching and orphan descendant IDs, positive
and deleted versions, normal/minimal/with-children entries, automatic and explicit
enhancement, and root slicing. Both sides use actual in-memory SQLite databases.

The initial comparison exposed 40 differences in 144 observations: native fields
merged correctly, but C++ dropped descendant columns outside the ancestor's
concrete model. Rust stores those columns in the generated dynamic map. Shared
`EntityRuntimeAccess::capture_dynamic_values` now validates readonly metadata
before native decoding and retains unknown columns in the existing model map.
It consults the cached Context field layout; it does not build an alias dictionary
or entity descriptor per row. Known native aliases and relations remain separate
from these extras and from durable dynamic-field metadata. The integration test
also verifies sparse serialization and explicit Global retention after deletion
of the source UserContext.

Rust's original `ensure_schema` additionally bootstraps Platform id 1 and its
active root. The C++ fixture explicitly seeds and selects that root. This setup
adaptation does not qualify automatic generated bootstrap, which remains a G12
responsibility. Raw-row requests and typed entity requests are compared separately
because the source applies child enhancement during typed loading.

All 144 actual SQLite observations now match. The 1800 construction observations
and existing scalar/optional/Checker/commit corpora also pass with the capture
adapter. This still does not establish return-type adaptation, streaming/failure
matrices or complete inherited graph/save behavior.

Context-allocated decode results and temporary Record containers must leave their
scope before that Context is deleted. They are staging values with allocated
payloads, unlike checked Q/E wrappers. The new sanitizer test caught a staging
value left alive past deletion; its scope is now closed before the deletion step.
The model is retained through explicit `global.make<School>(model)` before that
step, and the Global copy is the object subsequently serialized.

Final verification of the capture change covers 16 selected native checks,
including 1624 scalar/entity, 1548 optional-field, 1620 Checker-flow and 540
governed-commit observations. Fifteen passed in the initial final run; the
remaining lifetime test passed after its staging-scope correction. The targeted
sanitizer selection passes 3/3 across the final run and corrected test, with leak
scanning disabled; module consumption passes 2/2 across the same correction.
Production code and generated artifacts were unchanged during that correction.
Pinned formatting checks all 827 C++ files; all seven artifact sets reproduce.
