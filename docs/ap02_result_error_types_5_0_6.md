# AP02 Result error-type projection qualification

The generator repair is installed on `cpp_support` commit
`5a6c7c898eb30cd1e401e890e5152747da7255d9`. It preserves the error-type
requirements of `Result<T, E>` while resolving its successful entity type.
The runtime C++ implementation and its Context ownership contracts are unchanged.

The locked references remain Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Reproduced differences and repair

The [original diagnostic](parity/ap02_result_error_types_diagnostic_5_0_6.json.gz)
builds complete generated Rust libraries and C++ model facades. It identifies
seven Rust-accepted/C++-refused projections: errors represented by `Self`, a
qualified modeled entity, an integer, `Option<Self>`, a tuple and a mutable
reference, plus an owned Result projection. The unit-error control agrees.
Unknown or incorrectly scoped entities, unbound lifetimes and a non-cloneable
owned Result are refused by both implementations.

The resolver now retains every generic argument and the nominal paths needed
to validate its original module scope. Associated-type resolution retains
those requirements even when the resulting type omits an argument. Recursive
`Clone` and `Sized` checks distinguish these operations:

- `<Result<Self, &mut Self> as IntoIterator>::Item` resolves to the entity without
  requiring the error reference to be cloneable.
- Applying `ToOwned` to that Result first requires both arguments to be
  cloneable and is refused.
- Consuming the Result into its iterator first removes the error from the
  iterator's clone requirements while retaining the error's scope checks.
- A boxed slice can be a sized error value; an unboxed slice cannot.

The notation above describes the operations; the retained fixtures contain
their complete qualified Rust type syntax. These rules are generation-time
metadata interpretation and add no C++ runtime allocation or container.

## Actual qualification

The [copied candidate](parity/ap02_result_error_types_candidate_5_0_6.json.gz)
passes 178 Java tests and 28 actual original/C++ compiler pairs: 17 accepted
and 11 refused. The preceding implementation fails four of the five new
permanent regression methods. The candidate additionally checks nested error
paths, tuple/reference constraints, iterator ownership and invalid unsized
container construction.

The [generated SQLite consumers](parity/ap02_result_error_types_candidate_sql_5_0_6.json.gz)
execute six complete original/C++ model pairs with 30 matching query, E and
JSON results. Each executable is used only after its compiler and linker exit
successfully. The receipt preserves complete models, commands, dependency
identities, source hashes and outcomes.

The [production continuation](parity/ap02_result_error_types_installed_repaired_5_0_6.json.gz)
passes 178 Java tests and regenerates 84 complete SDKs. It performs 30 actual
C++ compiler processes and explicitly reuses 54 prior compiler outcomes
through complete SDK equality. All 57 accepted and 27 refused selections
match their retained locked-Rust outcomes.

The [first production attempt](parity/ap02_result_error_types_installed_5_0_6.json.gz)
stops after seven SDKs because its unconditional byte-equality check detects a
changed refusal for `<u64 as ToOwned>::Owned`. The same scalar type appears
under two historical selection names. Both remain invalid entity projections;
their changed C++ diagnostics receive fresh compiler checks in the continuation.
Their former compiler results are not counted as unchanged-artifact reuse.

All [80 SDKs](parity/ap02_result_error_types_sdks_5_0_6.json.gz) reproduce
3,560 unchanged implementation/Assist files. Each manifest refreshes exactly
the two reviewed producer fingerprints. The
[production SQL artifact continuation](parity/ap02_result_error_types_production_sql_reuse_5_0_6.json.gz)
freshly regenerates all six complete SQL consumer SDKs and matches their actual
execution artifacts. Its 30 runtime results are explicitly reused, with zero
new SQL executions claimed.

The [historical-source transition](parity/ap02_result_error_types_helper_transition_5_0_6.json.gz)
changes seven exact import/predicate boundaries in the preceding admission
helper. It preserves its other validation bodies and all previous controls.
The same receipt records one wording-only correction to the production recipe:
its primary scope text now states 30 actual and 54 reused compiler outcomes.
The immutable production execution already records these correct counts in
its compilation scope and individual case fields. The source admission checks
that exact single wording replacement instead of relabeling the executed file.

The [current controls](parity/ap02_result_error_types_admission_controls_5_0_6.json.gz)
reject 15 new and 42 preserved corruptions per Python mode. The
[actual registered checks](parity/ap02_result_error_types_native_5_0_6.json.gz)
pass 6/6 in each mode on 11,983 unchanged execution inputs. These are affected
caller checks, not a new full native epoch.

The [ordinary current source join](parity/sq02_result_type_source_join_5_0_6.json.gz)
and [optimized counterpart](parity/sq02_result_type_source_join_optimized_5_0_6.json.gz)
admit all 90 exact source transitions from the retained complete native
checkpoint. Both preserve the 1,101-test registry, 716 Python commands and all
707 native consumer hashes, and reject seven source-join corruptions per mode.
The initial controller referenced its future output instead of the preceding
container join and stopped before producing acceptance evidence. Its corrected
prerequisite retains the original container receipt and its exact hash.

The [installation milestone](parity/ap02_result_error_types_installation_milestone_5_0_6.json.gz)
checks the final source epoch and permits only `G14.evidence` and
`G14.open_questions` to change afterward. All capability verdicts are preserved.
Formatting and its check pass for 1,813 handwritten C++ files with pinned
clang-format 18.1.8; all execution input hashes remain unchanged. About 1.46 GiB
of measured owned probe outputs have been removed. Compact receipts are
retained with source, models, commands, dependency identities and outcomes;
stable native preset outputs remain available.

Complete AP01-AP12/G14, G17 and P0-P7 retain their independent exit criteria.
Capability status remains **14 qualified / 3 partial**.
The pending Option-model and column-attribute reference-defect decisions are
unaffected by this repair.
