# Runtime data-service helper source review: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: all 364 lines of `teaql-runtime/src/data_service/helpers.rs`, including
its two original cache-key unit tests. Its locked SHA256 is
`0244a34e8467f716d09ebcbbb5f3c2b2ade41ef3b41070e3c919fa5dc5613421`.
The [previous complete service review](runtime_data_service_api_contract.md)
is historical 5.0.5 evidence. This source remains pending on the current baseline.

## Complete responsibility map

| Review | Original responsibility | Current implementation and evidence boundary |
| --- | --- | --- |
| RH01 | Aggregate defaults and aliases | `relation_enhancement.cpp/.hpp` retains U64 zero for single results, an empty list for multiple results, and the requested alias versus the grouped `count` alias. Empty results still populate every parent in the empty-aggregate helper. |
| RH02 | Value bucket identities | `relation.cpp` retains each Value variant's prefix and representation, including typed-null/null, object/list sentinels, JSON, date, decimal and timestamp. Graph identity additionally merges nonnegative I64 and U64 identities; negative signed values remain distinct. |
| RH03 | Cache namespace, invalidation and key | `query_keys.cpp` retains entity namespace and the complete canonical query Debug key. Existing service cache adapters perform namespace invalidation without adding an external cache dependency. |
| RH04 | Recursive query canonicalization | Predicates, HAVING, expression projections/order, relation subqueries, object groups and child enhancements are recursively canonicalized. Only the source's aggregation-cache/comment/trace/stream/continuous-page fields are cleared. Purpose and other result-affecting state are retained. |
| RH05 | Recursive expression canonicalization | Function, binary, subquery, between, null tests, negation and boolean children retain their structure. IN/NOT-IN and large-list variants sort by cached Debug and deduplicate using equality. AND/OR flatten matching nested nodes, stable-sort by Debug and deduplicate. The C++ index tie-breaker preserves stable ordering without default-resource sort storage. |
| RH06 | Projection completion | The relation planner adds a missing key only to an already reduced projection; an empty projection remains the all-fields convention. Existing entries are not duplicated. |
| RH07 | Relation aggregate bucketing | The helper removes the grouping foreign key from child output, merges rows using graph identity, and attaches values using the separate parent-key snapshot. A single-cell row becomes that cell; multi-cell/empty rows remain objects. Multiple results retain their row order and default to an empty list. |
| RH08 | Initial versions | Insert/graph preparation recognizes only absent, Null, I64 zero and U64 zero as unassigned, then supplies I64 one for the descriptor's version property. Typed null, false, empty text and other types are not silently treated as zero. |
| RH09 | Timestamp defaults | Graph and ledger preparation use operational `Timestamp::now`, independently of BusinessClock. Create time is supplied only for new objects; update time may be supplied for either kind, and only for declared properties with the source unassigned values. |
| RH10 | Identity and target validation | Shared `graph_unassigned` and graph-write adapters retain the source missing/null/integer-zero definition and exact expected-versus-actual relation target errors. They do not merge provider, executor and runtime responsibilities. |
| RH11 | Version increments | Entity/ledger mutation paths retain original-version plus one or absent-version two. Debug preserves the overflow failure; Release uses unsigned arithmetic and bit conversion to retain wrapping without C++ signed-overflow undefined behavior. |
| RH12 | Ownership and independent layers | Nested rows, lists, keys, canonical queries and diagnostic text use explicit Context resources. Cross-domain retention needs a destination copy. Helper counterparts delegate through the existing runtime/service/executor/provider layers rather than implementing a second database path. |

## Changed source contract and discovered boundary

The only 5.0.5-to-5.0.6 change in this file adds `parent_keys` to
`attach_relation_aggregate_rows` and reads attachment identity from that
snapshot. This protects identity after another aggregate changes a parent field.
The current C++ snapshot overload implements that normal path but also rejects
unequal parent/key lengths. Rust uses `parents.iter_mut().zip(parent_keys)`:
it processes the shorter length and leaves any unpaired parents unchanged.
The source contains no equal-length check.

The isolated witness varies parent and snapshot lengths independently from zero
through three, scalar/list results and whether visible parent IDs have been
overwritten. Each parent starts with an existing aggregate sentinel; this makes
an untouched tail observable. Child rows include a grouping key and a payload,
so foreign-key removal and the single-cell/list shape remain visible as controls.
There are 64 cases, including 16 equal-length controls and 48 unequal-length
boundaries. The observer calls the actual private helper in a byte-checked
complete reference runtime view. Its added test is separate from the two
unchanged original cache-key tests.

The candidate removes the extra length rejection and bounds iteration by the
smaller span. It retains the snapshot lookup, child bucketing, Context allocation
and output conversion. Both spans remain bounds checked by the loop condition;
no raw access beyond the shorter span is introduced. The candidate is staged
outside the repository while the current full native epoch runs.

## Evidence and acceptance boundary

The [first witness receipt](parity/relation_key_zip_source_5_0_6.json.gz)
preserves an unsuccessful C++ observer compile: constructing `CompactRow`
directly requires explicit allocator arguments. The next
[observer attempt](parity/relation_key_zip_source_final_5_0_6.json.gz) preserves
a second harness error: `model_put` inserts rather than replaces a JSON key,
so preinserting a null error hid caught exceptions. Its candidate rows already
match all original outcomes, but its current-difference assertion fails and the
record remains unsuccessful. The observer correction stores the optional
failure first and emits the error field exactly once; no production JSON API
or exception behavior is changed.

The [corrected receipt](parity/relation_key_zip_source_verified_5_0_6.json.gz)
preserves the complete observer/controller/candidate, original source-view
provenance, commands, outputs and independent before/after input guards. Its
controller is terminal with exit zero. Both unchanged original tests and the
added caller test pass. Current C++ differs on all 48 unequal-length cases;
the staged implementation matches all 64 original outcomes. The 16 equal-length
controls retain both ordinary and overwritten-ID snapshot behavior. Frozen
execution inputs and both references remain unchanged.

The separate [imported witness](parity/relation_key_zip_module_5_0_6.json.gz)
uses a Runtime-only module consumer and the isolated candidate object. It
checks the same complete output against the source replay and guards every
linked archive. That controller is also terminal with exit zero: current
imported execution reproduces all 48 differences, and the staged imported
implementation matches all 64 actual-source outcomes. All linked archives,
frozen execution inputs and references remain unchanged. This provisional
consumer does not constitute permanent CTest registration.

## Permanent package and guarded installation

The [eleven-file package](parity/relation_key_snapshot_staged_package_5_0_6.json.gz)
contains the minimal implementation change, an explicit `<algorithm>` include,
an input-driven C++ probe, the original-body Rust caller, its 64-case corpus,
a comparator, a source verifier and native/module registrations. Its CMake and
module changes are based on the queued Event/Policy package's final contents;
they preserve that cohort's six new consumers rather than overwriting them.
Every proposed file has a before hash and complete after contents. Package
assembly is not installation or qualification.

The [package build](parity/relation_key_snapshot_package_build_5_0_6.json.gz)
compiles the actual permanent probe against current and candidate implementations.
Each frontend reproduces the current 48 failures and matches all 64 source
outcomes with the candidate. Both link only runtime and its lower layers,
without SQLite or its provider. The imported probe is prepared with the existing
canonical consumer tool and retains the complete source body. Default PMR is
disabled inside the probe; each input receives a fresh explicit Global/User
Context pair. Linked archives, execution inputs and references remain unchanged.

The [portable verifier replay](parity/relation_key_snapshot_portable_staged_5_0_6.json.gz)
freshly compiles the complete original runtime view with the retained caller.
Both original cache-key tests and the caller test pass; all 64 observations
match independently through Rust, the new native probe and the canonical
imported probe. Its explicit staged mode does not claim installed CTest gates.
After installation its default path builds and requires seven exact registered
tests per frontend, source replay, canonical body validation and selected no-work
rebuilds. It guards corpus, observer, implementation and reference hashes.

The [installation coordinator](parity/relation_key_snapshot_installed_cohort_final_5_0_6.json.gz)
waits for the verified Event/Policy coordinator to exit successfully. It requires
the preceding complete native epoch, both Event/Policy installed frontend gates,
the tested package hashes and exactly the expected execution-input changes.
Only then may it install the eleven files, apply/check the pinned formatter and
run the new verifier's default native/imported gates. Failure in a prerequisite
prevents installation; a waiting record is not installed or passing evidence.

The [snapshot-copy correction](parity/installation_snapshot_copy_correction_5_0_6.json.gz)
supersedes the first key-snapshot and service-type coordinators before either
installed files or launched child work. Their initial code retained the mutable
expected-input dictionary as `before_inputs`; later installation would have
changed that serialized before snapshot. The replacements store a separate copy.
Both old receipts remain unchanged, and their verified processes terminate with
143. The native and Event/Policy controllers continue uninterrupted.

The candidate is not installed, and this review does not promote a source or
capability. Permanent helper/corpus integration and native/imported regression
checks must follow successful completion of the current frozen native epoch
and the already queued Event/Policy installation. The broader data-service
types, execution, privacy, graph and P0-P7 responsibilities remain separate.
No sanitizer, MIPS or physical-device result is claimed.
