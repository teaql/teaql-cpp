# Progressive field and runtime Assist

This G12 checkpoint completes separate field discovery and supplies executable
schema, diagnostic and trusted-assembly examples. It does not declare G12/P6 or
P0-P7 complete. The remaining assembly/sample integration requirements stay in
[the runtime matrix](runtime_assembly_qualification_matrix_5_0_6.md) and
[the sample matrix](sample_qualification_matrix_5_0_6.md).

References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source responsibilities and discovery

The locked field Assist, schema Assist, debug Assist and runtime-custom Assist
are preserved in the [before snapshot](parity/assist_support_before_5_0_6.json.gz),
together with all 25 G12 source bodies and the previous 77 SDK file sets.
Generated `ASSIST.md` links canonical model names to
`assist/<cpp_entity_identifier>/fields/<field_identifier>.md`.
Reserved words retain their established C++ escapes; predicate vocabulary and
pluralization use the existing shared metadata helpers.

| Locked responsibility | Generated C++ guidance |
| --- | --- |
| Field selection, access and mutation | Exact scalar/FK/model/E methods, changed/eval, selection/order/group and aggregates; setters are explicitly not a client-writable allow-list. |
| Predicate families | Existing typed comparisons, set/range, Boolean, text and candidate methods; Known/Unknown aliases distinguish SQL NULL from E availability. Literal candidates occupying a Null/Known name retain their own meaning. |
| Relations | Forward/reverse getters, matching and presence; forward facets/details/roll-up and reverse-many counts/statistics use the actual source-selected method names. |
| Schema | An explicit `ensure_schema().wait()` through installed metadata, with failure propagation. Module installation and telemetry attachment remain passive. |
| SQL evidence | Independent select/mutation switches and a Context-owned safe projection containing operation, parameterized SQL, count, duration and result metadata. Bindings and rendered SQL are excluded. |
| Trusted customization | Generated configuration first; managed policy, audit sink, telemetry and tenant registration afterwards. Missing dependency fails readiness. Actual query/create examples exercise the configured runtime. |
| Input governance | Recursively reject the source's six case-insensitive governance keys in objects/arrays, retaining the offending key in the error. This helper is application boundary validation, not authorization. |
| Errors and telemetry | Retain typed runtime errors before optional exception unwrapping; commit/audit-delivery semantics stay distinct. Telemetry remains an application-owned SPI. |

The three runtime code examples define ordinary application functions, not new
SDK APIs. The sample policy remains permissive and is labeled accordingly.
Applications supply real tenant/authorization rules. Safe audit delivery remains
after commit and separate from immutable raw audit; the sample sink is not an
outbox. No shared ownership or hidden default allocator retains Context data.

The filename configuration example uses the existing `configureRuntime`
convenience. Conditional Business-ID key/service setup and environment-driven
`serviceRuntime` configuration keep their separate, model-aware index guidance
and [configuration contract](business_runtime_configuration_5_0_6.md).
Showing a relationship statistic in field help does not qualify G08: its
[remaining result differences](aggregation_qualification_matrix_5_0_6.md) stay
explicit requirements of the full generator plan.

`SafeSqlEvidence` propagates its allocator through nested strings and vector
copies. It retains native unsigned seconds/nanoseconds instead of narrowing the
source's `u128` microseconds to a signed C++ counter. Result and event borrows
expire with the Context. The managed policy explicitly declares its allocator
contract even though it has no payload: a virtual destructor makes it
nontrivial, so it is not eligible for the kernel's trivial-object allocation path.

## Separate Tool package boundary

The locked Tool Assist targets `teaql-tool-context` 2.0.0, a separate package,
not the locked TeaQL 5.0.6 kernel. Its time/id/text/json/decimal/money/HTTP facades
are not C++ Context methods. `assist/runtime/tool_api.md` identifies the boundary
and instructs the agent to report `MISSING_ASSIST` for an unavailable operation.
The [frozen dependency evidence](parity/assist_support_tool_boundary_5_0_6.json.gz)
retains the Tool Assist inventory and complete workspace/runtime/library Cargo
inputs; the latter do not include that package.
Existing native core facilities are not relabeled as that external facade.
No HTTP endpoint, cloud integration, OpenTelemetry exporter or generated
application document adapter is claimed. The source's document example itself
leaves its domain adapter application-owned.

## Validation boundaries

The [initial Java record](parity/assist_support_java_5_0_6.json.gz) preserves the
intended missing-document regression and a corrected KSML test fixture. The
fixture initially used `boolean()?`, which is a String candidate declaration;
the actual optional Boolean declaration is `bool()?`. The original failure is
retained separately; production type interpretation was not changed.

The first actual example compiler found that the policy lacked the allocator
contract required by `user_context.make<T>()`. SDK-only controls compiled,
isolating the error to the new documentation. Its
[failure record](parity/assist_support_compiler_initial_policy_failure_5_0_6.json.gz)
retains both results; redundant checks were cancelled after reproduction and no
cancelled compiler output was reused. The generator template fixes the example
without weakening `ArenaObject` or changing runtime allocation.

An earlier compiler-driver argument shadowing failure is also retained in
[its own record](parity/assist_support_compiler_initial_driver_failure_5_0_6.json.gz).
It failed before checking SDK examples and does not count as a compiler pass.

Generation, strict reproduction, actual C++ compilation and native/imported
SQLite execution must all be terminal before this checkpoint is qualified.
Method-spelling and link checks establish discoverability only; they do not
replace the accepted G06/G07 runtime semantics or complete G12 qualification.

## Completed generation and execution gates

The [final Java record](parity/assist_support_final_java_5_0_6.json.gz) passes
74 tests after the allocator-contract repair and expanded field assertions.
The [producer preservation proof](parity/assist_support_producer_scope_5_0_6.json.gz)
restores every previous producer byte after removing only the reviewed new
document/index/guidance emission. Existing action generation is byte-identical.

Real generation adds 1,077 field documents and four runtime documents per SDK:
1,329 new documents in 63 successful SDKs. All 77 destinations strictly reproduce
(24 ordinary workspaces, 50 contract destinations and three identity fixtures).
All old C++ code, catalog data and 1,253 action-example bodies remain unchanged;
14 rejected-model destinations retain their original outcomes. The
[regeneration proof](parity/assist_support_regeneration_5_0_6.json.gz) preserves
all file hashes, count envelopes, commands and terminal outcomes.

The [compiler gate](parity/assist_support_compiler_5_0_6.json.gz) compiles the
actual three complete runtime blocks in every successful SDK: 63/63 sets and
189 blocks. It uses the same shared-production-header PCH as the prior action
gate, with each model SDK compiled independently. No failed SDK-only baseline
is hidden. The [field/link check](parity/assist_support_field_links_5_0_6.json.gz)
finds no absent method spellings in 1,077 field guides and resolves all 1,329
local document links. That mechanical check does not establish semantics.

Actual SQLite execution passes native 2/2 and imported modules 2/2, using the
same extracted header and the same handwritten test bodies. The
[execution record](parity/assist_support_execution_5_0_6.json.gz) retains complete
logs, source and terminal-built executable hashes. Checks cover passive schema
configuration, explicit schema/readiness, trusted dependency failure, actual
schema and business audit events, SQL evidence/allocator propagation, independent
logging switches, six recursive governance denials and full-range native duration.
The prior action test also exercises actual create/query/update/delete/page/
stream/E operations. No production runtime algorithm or dependency changes.

Two fixture errors are retained separately. The
[first fixture compiler failure](parity/assist_support_fixture_initial_compile_failure_5_0_6.json.gz)
passed an allocator twice to PMR vector emplacement. The corrected fixture lets
uses_allocator supply the Value allocator. The
[schema audit diagnosis](parity/assist_support_schema_audit_fixture_failure_5_0_6.json.gz)
records the initial incorrect assumption that startup left the sink empty.
Actual debugger observation counted ten callbacks; source inspection identifies
schema-created/verified events before the business event. The fixture preserves
and checks those events, then checks the Created-event delta instead of clearing
or suppressing startup audit.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_assist_support_tests teaql_assist_workflow_tests
ctest --preset native --output-on-failure -R '^(assist_support_contracts|assist_workflow_contracts)$'
cmake --build --preset modules_native --target teaql_module_assist_support_tests teaql_module_assist_workflow_tests
ctest --preset modules_native --output-on-failure -R '^(module_assist_support_contracts|module_assist_workflow_contracts)$'
```

G12 remains partial. Complete assembly/sample/source responsibility review and
final integrated qualification remain required alongside G08/G13-G17. This
checkpoint adds no sanitizer, MIPS, full-suite or physical-device execution claim.

Final pinned formatting and its check pass for 1,644 handwritten C++ files.
The locked-source and module-path checks pass; 792 canonical imported bodies
retain their registered transformations. These mechanical checks do not replace
the separately recorded execution gates.
