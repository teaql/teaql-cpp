# Execution source/result model responsibility: Rust 5.0.6

EX13 is qualified for the locked generated execution interface, with the existing
C++ ownership and execution-timing adaptations. This does not qualify the whole
G09 capability, model/storage capabilities or P0-P7.

References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Complete responsibility and evidence

| Responsibility | Source behavior, native mapping and evidence |
| --- | --- |
| Preserve the source request | Original `return_type<T>` transfers the complete query/options/relations/children without changing the source entity. Native `returnType<T>` preserves Source and Executor template parameters and the checked description/binding; lvalues copy the description and rvalues transfer it. All source queries and first Policy snapshots remain equal across target choices. |
| Decode the requested result | List/first/one/page/stream use R for entity decoding. The source entity still selects the repository, metadata, SQL and request intent. The new full JSON/load-state matrix has 2,080 typed observations, rather than comparing only IDs/versions. |
| Keep compact/scalar entries independent of R | Public rows/count/exists retain their Entity-result guard but never decode R. All 1,248 observations compare complete results, errors, Policy and diagnostics across the four target choices. Count/exists keep their own source query transformations. |
| Preserve selected, omitted and readonly values | Normal/minimal projection, raw field projection, computed readonly data and facet metadata cross four source/target models. A separate 64-case source execution distinguishes selected readonly NULL, omitted target fields and required scalar defaults. |
| Preserve errors and authorization | All 256 deny/reject-raw cases fail before SELECT or SQL diagnostics. Full errors and safe log fields compare without normalization. Source/result adaptation does not change which entity's request Policy reviews. |
| Preserve explicit ownership | All native cases disable default PMR and actually delete their UserContext. Existing current source/result contracts verify independent lvalue requests, explicit Global copies/rebinding, source-model creation and expired query/E rejection; all-eight foreign/expired binding controls still pass. |

The complete source bodies remain in the
[current execution review](parity/execution_responsibility_after_lookup_results_5_0_6.json).
R is an unconstrained construction marker until the public execution/creation
Entity guard. It participates in typed decoding, not repository selection or
scalar/compact execution. Primitive field decoding, inheritance, constants and
candidate semantics retain their independent G01/G04/G05/G07 contracts; this
review does not replace those responsibilities with a smaller model inventory.

## Actual unchanged-source matrix

The new observers call the complete unchanged inheritance-graph library and real
Context/service/executor/SQLite implementations. No query or decoder algorithm is
implemented by the observer. The declared 3,328 inputs cross School, Academy,
ResearchAcademy and Enrollment as source and result independently, normal/minimal
entries, all eight execution methods, ordinary/ID-set/continuous configuration,
and plain/readonly/facet/raw-field projections. Policy denials supplement the
successful matrix. There are 2,496 conversions between different source/result
types; complete results, 3,968 Policy snapshots and 4,096 stable SQL records agree.

Snapshots retain every target descriptor's property/relation loaded flag, its
complete native JSON and computed-value observation. Unknown selected columns
retain the original additional-property behavior. `returnType<T>()` does not
load another table, synthesize omitted fields or grant persistence authority.

The existing generated-helper behavior still applies: typed list/page/first/one
execute facets, while compact/scalar/stream entries do not run that attachment
layer. The new observer retains the complete query and facet SQL/Policy effects;
complete facet-map attachment has its existing independent source/deferred
acceptance. No unobserved facet payload equality is inferred here.

## NULL and omitted fields

The additional 64 cases use minimal projections across disjoint and inherited
layouts, through all eight entries. Selecting `_computed = NULL` preserves the
explicit JSON key with NULL; omitting a target field leaves its loaded flag false.

Forced SQL NULL for a declared required generated string follows the original
materialization rule: the field is loaded with its default empty string. When
that column is not declared by the chosen target, its additional projection stays
NULL in JSON. Both outcomes are retained exactly. The comparator does not rewrite
NULL to a default. This raw-projection rule does not redefine nullable-field or E
Value/Null/NotLoaded contracts.

The NULL-capable original observer also reproduces all earlier 3,328 inputs
without changing their corpus, source observer or generated files. Complete raw
paired outputs remain in the [main comparison](parity/execution_result_models_final_5_0_6.json.gz)
and [NULL comparison](parity/execution_result_model_states_initial_5_0_6.json.gz).

## Verification and remaining scope

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_execution_result_models_probe
ctest --preset native --output-on-failure \
  -R '^execution_result_model(s|_states)_rust_parity$'
```

The [terminal record](parity/execution_source_result_models_terminal_5_0_6.json.gz)
binds complete source/native/module results, original dependency identities,
source hashes and current ownership/binding checks. No production, generator or
generated artifact changes are required. Existing source/error/lifetime
adaptations remain explicit; unchanged sanitizer/MIPS evidence is not relabeled
as a new run.

Both comparisons pass: 3,328/3,328 full model observations and 64/64 NULL-state
witnesses. The imported consumer passes 2/2; current native binding/ownership and
retained source-result checks pass 3/3, including the 702-case historical corpus.
Formatting/check covers 1,640 C++ files and canonical module verification covers
791 bodies. Source locks, generated manifests and 1,495 catalog exports verify.

EX13's source/result separation is closed. Full G09 still needs the remaining
execution/configuration and assembly responsibility reconciliation and integration
exits. Broader provider, dynamic-storage, privacy/governance, model and P4/P7
requirements remain with their original capability owners. No physical-device
or external-provider acceptance is implied.
