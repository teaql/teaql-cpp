# G12 current source and evidence audit

References remain Rust 5.0.6
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.
This audit reconciles the eighteen finite responsibility groups with actual
source behavior and evidence. It does not promote G12/P6 or P0-P7 while R08 and
current integration remain open.

Current integration note: the formerly running frozen native epoch is terminal
735/736 with its failure preserved. Subsequent focused follow-ups do not turn it
into a full current pass. The native registry is now 738. The later
[complete Language/I18n qualification](i18n_language_source_review_5_0_6.md#installed-source-qualification)
adds current native 4/imported 4 and 4,635 actual-source outcomes per frontend,
including generated vocabulary/Checker installation; it supports R17 without
closing R08 or complete assembly acceptance.

## Evidence integrity and execution scope

All 28 source bodies and 22 primary evidence records in the
[earlier reconciliation](parity/g12_current_responsibility_reconciliation_5_0_6.json.gz)
retain their recorded SHA-256. The current C++ producer's runtime, registry,
sample-recipe and Assist implementation was inspected against the complete
locked runtime/library/sample templates and their dispatch drivers. This is a
responsibility review; matching an anchor or method name alone is not acceptance.

The recorded native and imported integration logs were independently parsed.
Every one of the 51 native and 40 imported selected test names has exactly one
passing terminal outcome, including the separately executed whole bootstrap
case. There are no skipped or missing cases in those records. The two selections
remain different sets, not interchangeable coverage. This validates those
historical executions rather than claiming the currently rebuilding binaries
have already passed.

Five existing strict provenance checks also pass: Business-ID samples,
customized samples, constant/candidate samples, configured scalar samples and
Business-ID runtime configuration. Their original SDK/model/observer inputs and
reference registry identities retain their contracts.

The broad `rust_field_layout` observer workspace has 46 registry
name/version/source/checksum tuples outside the frozen reference lock. This
does not invalidate its preserved outputs automatically, but its old lock is
insufficient proof of exact-reference execution. A temporary minimal workspace
therefore replays five complete retained corpora with unchanged original
observer bodies and five unchanged original generated domain libraries. It is
seeded from the reference lock, verifies registry identities before compilation
and builds offline with `--locked`.

The [current audit receipt](parity/g12_current_source_audit_5_0_6.json.gz)
retains the checks, exact old dependency differences, manifest/lock, source
hashes, full replay outputs and terminal commands. Its `complete` field marks
whether that replay has finished. Existing workspaces and corpora remain
unchanged. No current native executable is inspected or run during the frozen
consumer build.

The replay is terminal and passes **347/347** complete source outcomes:
72 ordinary assembly, 144 sample-state, 18 real sample execution, 28 failure/
continuation and 85 scalar/temporal/JSON cases. They use the original retained
inputs and outputs without normalization. The last family uses the existing
fixed process-clock fixture and checks its observed wall time before comparing
results. Neither the generated Rust libraries nor historical corpora change.
This closes the exact-reference dependency replay gap for those five families;
it is not a new C++ execution or whole-suite result.

## Finite responsibility dispositions

| Group | Source behavior checked | Evidence disposition and retained boundary |
| --- | --- | --- |
| R01 | Library exports, domain wrapping and progressive discovery | Generated model/Q/E/runtime/sample exports and canonical Assist discovery are mapped. Existing workflow and support compiler/execution records remain the behavioral evidence. Source export inventories alone do not prove application behavior. |
| R02 | Config, environment, provider preparation and typed errors | Preserve missing versus empty/non-Unicode environment values, path preparation, SQLite errors and their nested sources. The filename convenience and source-shaped configuration entry remain separate contracts. |
| R03 | Shared connection, aliases and explicit ownership | DataServiceConnection and explicit Global/User registration replace Arc pool ownership. Existing clone/rebinding/expiration/cleanup tests qualify that adaptation; no implicit owner extends Context lifetime. |
| R04 | Conditional Business-ID allocator, managed keys and startup order | The qualified three-entry startup repair retains conditional constants, key validation, pre-executor failure and actual filesystem/save/readback results. Source panic/C++ exception cases remain explicitly disposed, not rewritten as successful startup. |
| R05 | Independent schema provider and domain executor delegation | Fresh static model descriptors remain independent of mutable Context metadata. Query, dynamic-field store, stream, mutation and transaction forwarding retain separate runtime/executor/provider roles. External search/provider branches remain outside scope. |
| R06 | Three standalone registries | Repository membership, default behaviors and typed Checker adapters remain independent factories. Existing default/custom replacement and checked ownership evidence is retained; constructing a registry does not install it or execute SQL. |
| R07 | Four module modes and governed bootstrap | Source-shaped ordinary/Checker/Behavior/combined installation stays passive until schema invocation. The 72-outcome original replay preserves repeated initialization, application edits and the original relationship-schema failure. Broader governed caller responsibilities remain G13/G17 dependencies. |
| R08 | Own-field wire maps and separate empty aliases | Three valid naming profiles retain original own-field mappings without inherited flattening or relation-to-FK rewriting. Invalid json_field_naming remains an explicit unresolved difference: source succeeds with empty mappings while C++ rejects. |
| R09 | Sample plan/state/ID conversion/reference selection/report | The complete 144-outcome source replay covers all three scales, duplicate/zero/max IDs, used-reference fallback, skip/report order and optional Value-to-ID conversion. These are state semantics, separate from persistence. |
| R10 | Root/constant loading and dependency order | Root and constant loads retain the original empty-list fallback on load failure. Depth/name ordering and root/constant/business partitions come from the source driver. Full execution/failure corpora and constant/candidate sample evidence remain distinct. |
| R11 | Fanout, required dependency skip and reverse publication | Preserve max(1, object-fields) times scale, required skips before generation, reference reuse and publication only when reverse relations exist. Sorting reverse relations does not imply traversal absent from the source template. |
| R12 | Scalar, temporal, JSON, reference, configured and Business-ID recipes | Exact original operands remain distinct from typed native presentation. The 85-outcome fixed-clock replay complements six configured-type domains and Business-ID sample records. The existing source compiler failure remains a failure, not a reason to fabricate a working source API. |
| R13 | Real audited Save, partial progress, retries and cleanup | Retain earlier committed rows when a later save fails and source ID sequencing on repeated invocation. Original execution/failure outputs and the existing suspended/cancelled/native cleanup tests serve different obligations. |
| R14 | Every sample Info site and custom callbacks | Start/entity/progress/skip/success messages retain order. Source log level/target records stay in original evidence; C++ uses the documented Context-owned sample sink. Sampling/log switches do not authorize persistence or replace audit delivery. |
| R15 | Seven action workflows and field-specific guidance | Actual extracted create/query/update/delete/page/stream/E examples, canonical field links and the accepted dynamic-JSON grammar remain the API evidence. The example functions are application code, not invented SDK methods. |
| R16 | Schema, safe diagnostics, trusted customization and telemetry guidance | Preserve passive installation, explicit schema/readiness, recursive governance-key rejection and independent logging switches. Applications own authorization, sinks and document projections. External exporters and generated application document services are not claimed. |
| R17 | Application vocabulary and locale/Checker installation | Reuse the unchanged shared catalog generator. Existing generated catalog/Checker and explicit Global retention results remain separate from generic language/catalog source acceptance. |
| R18 | Separate Tool package availability | Rust Tool Assist targets an external package absent from the kernel lock. C++ guidance reports unavailable operations instead of inventing HTTP/money/text facade parity. The explicit external boundary is not a missing in-scope kernel implementation. |

The current shared runtime machinery is inspected before its thin generator
delegates. Sample recipes remain model-specific data/accessors; the runtime
owns polling, iteration, progress, persistence and cleanup. Configured raw
operands are not silently converted merely to make a typed sample succeed.

## Remaining acceptance

Preserve the terminal source replay and historical selection proof before
changing formal capability status. The failed 735/736 epoch is terminal with
its frozen inputs and accounted outcomes preserved. Complete current native
integration remains open after the subsequent production and test changes.
R08 still needs its already requested behavior disposition. Existing broader
G08/G13-G17 and P7 requirements remain independent, and source-file dispositions
are not promoted by this G12 audit. No new Java, generator reproduction,
sanitizer, MIPS or physical-device run is claimed for this read-only work.

The finite source-responsibility audit is complete: each of R01-R18 now has a
specific implementation/evidence disposition above. Formal row/capability
promotion remains pending current integration and R08; it does not require an
indefinite new set of models. A durable reference-locked observer for the five
G12 assembly/sample families is now installed in the continuation below, preserving the old broad workspace and its
dependency-drift record. New source defects would justify additional work;
unchanged green selections do not need repetition merely for this audit.

## Durable observer and canonical comparison continuation

The [dedicated source project](../tests/parity/rust_g12_source_5_0_6/Cargo.toml)
and [repository helper](../scripts/verify_g12_source_5_0_6.py) now retain the four
unchanged source observers and five generated Rust SDKs for all five families.
Repository-relative paths replace temporary absolute locators. Registry/package
identities, clean runtime/generator references, complete SDK/model provenance,
observer/fixture/corpus hashes and current clock-source provenance are checked
before execution. The older broad workspace and its 46 dependency differences
remain untouched.

All four complete existing C++ observation bodies now have canonical imported
targets and entries in the module preset. Their five comparisons are permanently
registered in module CTest, including the clock-controlled scalar/temporal/JSON
matrix. The current canonical checker verifies 913 consumer/fixture bodies.
The current installed native selection passes **5/5**, with the real sample
execution family taking 263.42 seconds. Its earlier full-suite duration was
628.896 seconds; the new module/source-helper budget is 1,800 seconds for that
family, retaining all 18 inputs and exact comparisons. No input or expected
outcome is removed to meet a shorter budget.

The [initial pipeline](parity/g12_durable_installed_gates_5_0_6.json.gz) retains
the successful native 5, coherent module compilation and four passing imported
families. Its fifth imported family is stopped by the stale configured 120-second
budget. This failed checkpoint remains incomplete. An explicit reconfigure and
[effective-property check](parity/g12_effective_module_budget_5_0_6.json.gz)
now confirm 1,800 seconds. The
[follow-up](parity/g12_durable_installed_followup_5_0_6.json.gz) reruns only the
timed-out comparison, which passes in230.29 seconds, and records unchanged
before/after identities of all eight completed native/imported observation
executables. Current coverage is native5 and imported4 plus retry1; the initial
failed imported run remains failed.

The helper's explicit `--original-only` mode now successfully replays fresh
Rust outputs against all 347 unchanged expectations. The completed native/imported
comparators independently establish C++ equality with those same expectations;
the join reuses their current terminal evidence rather than rerunning unchanged
expensive C++ matrices. The record states that C++ is not executed in this mode.
The default helper still supports a direct original/native/imported replay.
The [source replay](parity/g12_durable_source_replay_5_0_6.json.gz) and follow-up
are terminal and successful. Their `complete` flags and exit codes retain the
actual outcome; a live job or configured target is not a passing result. The
[budget record](parity/g12_durable_budget_5_0_6.json.gz) preserves the historical
timing basis. The initial
[fixture-locator preparation failure](parity/g12_durable_initial_fixture_locator_failure_5_0_6.json.gz)
is also retained; it was corrected to the existing rust_values helper before
any module configuration or runtime execution, without changing an observer.

No producer, generated SDK or handwritten C++ implementation/observation body
changes. R08's unanswered disposition and complete current integration remain
required; this continuation does not promote G12, G14 or P0-P7.

The [terminal evidence join](parity/g12_durable_source_join_5_0_6.json.gz)
binds the current corpus/comparator/binary identities and all five retained
source outcomes with the preserved native/imported executions. The explicit
original-only replay avoids repeating the same already-passing expensive C++
matrices; it does not reduce their tested inputs or weaken comparison semantics.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_runtime_assembly_probe \
    teaql_generated_sample_state_probe teaql_generated_sample_execution_probe \
    teaql_generated_sample_fields_probe teaql_sample_fixed_clock
cmake --build --preset modules_native --target teaql_module_generated_runtime_assembly_probe \
    teaql_module_generated_sample_state_probe teaql_module_generated_sample_execution_probe \
    teaql_module_generated_sample_fields_probe teaql_sample_fixed_clock
ctest --preset native -R '^generated_(runtime_assembly|sample_state|sample_execution|sample_failures|sample_fields)_rust_parity$'
ctest --preset modules_native -R '^module_generated_(runtime_assembly|sample_state|sample_execution|sample_failures|sample_fields)_rust_parity$'
python3 scripts/verify_g12_source_5_0_6.py \
    --module-build-dir build_modules_native --record /tmp/g12_source_fresh.json.gz
```
