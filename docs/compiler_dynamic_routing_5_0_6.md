# Dynamic C++ generation routes, Rust 5.0.6

## Confirmed source boundary and implementation plan

The trusted Spring configuration installs the compiler-backed `cpp-lib-core`
source generator. `/generate` passes uploaded models to that service. The
ordinary dynamic POST path first calls `TemplatePreviewService.validateTargetSafe`.
Its whitelist omits C++, and its existence check assumes every generator has a
StringTemplate resource directory. `CppCoreGenerator` generates files in code.
Consequently, `/cpp-lib-core` is rejected before the installed compiler runs.

1. Reproduce multipart and raw-body dynamic requests against the current
   controller and a controlled source-generator callback. Retain the actual
   rejection, zero callback count, exact source images and invocation results.
2. Recognize the existing canonical C++ generation scope as a code-generated
   target. Preserve resource checks for template-based targets and existing
   model evaluation, source snapshots, capability assembly and version selection.
   Keep explicitly unconfigured generation separate from the configured route.
3. Add permanent contracts for both dynamic input forms, unconfigured ordinary
   generation and configured compiler refusal. Extend the controlled host probe
   to exercise those same public routes. Use the existing Servlet interfaces
   and generator dependencies.
4. Run fresh complete locked-original/C++ compiler jobs through both routes.
   Execute generated typed SQLite/Q/E and governed Save/sink consumers. Preserve
   original refusals, complete source packages and SDK identities; do not infer
   that template preview or server Assist is implemented by this route repair.
5. Qualify exact source transitions, current Java contracts, relevant native and
   receipt/source guards. Preserve retained SDK bytes, source/profile boundaries
   and failed attempts. Reclaim owned compiler/class outputs after child exit,
   retain compact evidence and publish without promoting G13/G14/G17.

This is generation-host routing. The C++ edge runtime remains SQLite-only and
receives no Java, Rust compiler, web or additional runtime dependency. Runtime
Context, query, mutation, Policy and audit responsibilities remain separate.
Complete configuration routing, redundant interpretation, pending E03/E04
choices and final P0-P7 exits retain their own acceptance requirements.

## Reproduced boundary and Java regression

The [initial controller diagnosis](parity/compiler_dynamic_route_initial_5_0_6.json.gz)
invokes actual dynamic multipart and raw-body methods with a controlled installed
source-generator callback. Both requests fail with `SecurityException`, return
zero ZIP bytes and invoke the callback zero times. The failure occurs before
compiler dispatch, rather than during original model compilation.

`TemplatePreviewService` now recognizes the existing `CppCoreGenerator.SCOPE`
in its allowed targets and checks that code-generated scope separately from
template directories. Model evaluation, source snapshots, capability installation
and compiler failure handling retain their original owners. The controlled host
probe adds explicit dynamic upload/body options while preserving its existing
service and `/generate` options.

The [first Java gate](parity/compiler_dynamic_route_units_5_0_6.json.gz)
passes the ten host contracts, then records three missing-fixture failures in
the complete 275-test selection because its launcher uses the runtime checkout
as the working directory. The [corrected gate](parity/compiler_dynamic_route_units_repaired_5_0_6.json.gz)
uses the producer's `generator` directory: all **275 selected generator tests**
and **seven preview/target tests** pass. Both actual dynamic controller inputs
dispatch exactly once and return ZIP bytes. The permanent tests also preserve
ordinary unconfigured generation and ensure configured compiler refusal returns
no fallback SDK.

The exact preceding `TemplatePreviewService` is compiled separately and placed
first on the same test classpath. It fails precisely the three new host route
regressions and the source-free C++ target validation. Other preceding host
contracts still pass. Both outcomes, current source images, commands and owned
directory cleanup remain in the corrected receipt. These are in-process
controller/host tests; no HTTP server is deployed.

## Actual compiler, SQLite and publication results

The [complete configuration](parity/compiler_dynamic_route_complete_5_0_6.json.gz)
uses dynamic multipart upload. The
[partial-effect configuration](parity/compiler_dynamic_route_partial_5_0_6.json.gz)
uses the raw ZIP-body route, including the normal `/teaql/` prefix. Both retain
the existing source-derived School configurations and the original business
observer bodies. Only the copied host adapter's project-root binding and explicit
route argument change. No generated source is patched.

Each complete original SDK contains forty-three files and its C++ SDK contains
ninety-six. Each original library supplies twenty compiler-resolved projections
and complete compiled descriptor/scalar observations. All forty-one recorded
execution commands per configuration pass. The eight additional typed relation
descriptors, audit declaration and effective limits retain their source effects:
17 for complete parsing and 13 after the malformed middle expression.

All **twenty-four paired Save results** match, covering single-entity and mixed
pending graph writes, whole-plan denial/approval, later transport failure,
optimistic conflicts and post-commit audit-delivery failure. Authoritative root
and child rows, versions, audit fields and plan sizes remain distinct. Both
actual typed SQLite query results and masked SQL diagnostics also match.

The [multipart sink matrix](parity/compiler_dynamic_route_complete_sinks_5_0_6.json.gz)
and [body sink matrix](parity/compiler_dynamic_route_partial_sinks_5_0_6.json.gz)
pass all **twenty-four publication observations** using the existing ordinary,
sensitive and debug-destination controls. This is actual file publication over
synthetic test data, not a new production sink or an authorization change.

The [actual negative job](parity/compiler_dynamic_route_original_refusal_5_0_6.json.gz)
appends a source-valid Boolean projection to the same modeled School. Dynamic
body routing reaches the locked original compiler, which returns E0277 because
`bool` does not implement `Entity`. The host propagates the compiler refusal,
returns no C++ SDK and cleans its service/compiler jobs. This independently
checks real compiler refusal beyond the controlled unit worker. It neither
adds a compatibility exception nor decides E03/E04.

## Source admission and retained execution

The [first compatibility check](parity/compiler_dynamic_route_validation_5_0_6.json.gz)
passes seven readers, then rejects the preceding Date consumer receipt. That
receipt predates its result reader, so its source-frame addition also needs the
exact qualified reader transition. The
[bounded continuation](parity/compiler_dynamic_route_reader_continuation_5_0_6.json.gz)
changes only that addition guard: supplied current bytes must still match the
current file, and the historical hash must reach it through registered images.
The original source receipt and failed command remain immutable.

The active admission entry is:

```sh
source scripts/toolchains/activate.sh
python3 scripts/compiler_dynamic_route_source_admission_5_0_6.py --check
python3 -O scripts/compiler_dynamic_route_source_admission_5_0_6.py --check
```

It validates complete retained compiler/business records rather than executing
those business programs again. Fresh corruption reports use `--controls` with
a new output path. All acceptance decisions are unconditional in both Python
modes; a forged current added-reader hash remains rejected.

The [repaired reader cohort](parity/compiler_dynamic_route_validation_repaired_5_0_6.json.gz)
passes all twenty-two ordinary/optimized source and result-reader commands on
unchanged source/native/reference inputs. Each Python mode rejects 196 source
and receipt corruptions. The
[imported-helper continuation](parity/compiler_dynamic_route_imported_helpers_repaired_5_0_6.json.gz)
adds four direct rejections per mode, including independently incorrect
committed storage even when both language observations are changed together.
The total is **200 rejected corruptions per Python mode**. The earlier imported
run retains its preceding reader-source frame separately.

The [two selected native CTests](parity/compiler_dynamic_route_native_source_5_0_6.json.gz)
pass with exact registry/JUnit membership and stable artifacts. Their actual
execution precedes the final addition-guard continuation; exact source admission
retains that execution without repeating the native or business programs. This
checkpoint adds no full native/module/sanitizer/MIPS or physical-device epoch.

The [first milestone aggregation](parity/compiler_dynamic_route_milestone_initial_5_0_6.json.gz)
finds one additional source-frame mismatch: this new admission leaf itself
changed after those two CTests. All native artifacts and other qualified inputs
remain unchanged. The
[leaf history](parity/compiler_dynamic_route_leaf_history_5_0_6.json.gz)
recovers the exact old source by reversing the recorded additions; its hash
matches both the actual native receipt and first failed reader cohort. The
current source matches every input of the passing twenty-two-reader cohort.
Only the three identity/registration/control functions and two new history
functions change. The
[metadata qualification](parity/compiler_dynamic_route_leaf_history_validation_5_0_6.json.gz)
accepts this exact join and rejects six corruptions in each Python mode.
This preserves actual execution and the failed aggregate without editing an
active verifier or repeating native/business tests.

The [final development checks](parity/compiler_dynamic_route_development_5_0_6.json.gz)
apply and check clang-format 18.1.8 over 1,820 handwritten C++ files, verify the
locked upstream, active module paths, capability ledger and both repository
diffs. Source, native artifact and reference frames remain unchanged. The
[milestone](parity/compiler_dynamic_route_milestone_5_0_6.json.gz) binds these
commands, exact source history, retained reports and the pushed producer
checkpoint. G13/G14/G17 remain partial; the full P0-P7 goal remains active.

The [retention receipt](parity/compiler_dynamic_route_retention_5_0_6.json.gz)
preserves all eighty SDK manifests and 3,560 generated files, plus all 1,824 C++
source/fixture inputs. Nineteen exact owned paths are absent; fourteen distinct
measurements total 1,537,901,381 bytes reclaimed. Shared caches and stable preset
outputs remain in place. Actual failed launchers and compact source/command/
outcome evidence are preserved.

## Remaining interpretation routes

The inspected entry points retain these distinct responsibilities:

| Entry | Current configuration authority |
| --- | --- |
| Configured Spring `/generate` and dynamic `cpp-lib-core` POST | Original source bytes are snapshotted and sent to the installed compiler. Full v4 facts provide descriptors, semantic/scalar storage and projections. |
| Configured Domain-only service call | Rejects the absent original source files; it does not fall back to a different generator. |
| Unconfigured Spring/service construction | Retains class discovery and the existing Java interpretation path. Its complete embedded-Rust responsibility remains open. |
| Default offline `CppCoreGenerator.main` | Parses the model and uses the unconfigured Domain-only service; qualification flags also select explicit interpreted emitters. |
| Explicit compiled-model CLI | Consumes a supplied, versioned, model/toolchain-bound resolution document. Projection-only versions retain their declared responsibilities; they do not silently become full metadata capabilities. |
| Server Assist/template preview | Remains separate from source generation. This repair adds no C++ server Assist templates or template previews. Generated SDK guidance retains its existing qualification. |

In the full compiled path, `CppCoreGenerator.auditMetadata` reads the supplied
configuration, descriptor property shapes use compiled facts and scalar storage
uses the observed getter identity. The projection resolver uses compiled owners.
The corresponding null/older-capability branches still invoke their documented
interpreters. These source facts locate the next Stage 5 routing/redundancy
review; they do not prove its completion or authorize an input-language change.

## Next delivery and acceptance order

1. Map each remaining public/offline entry to its source owner, original input
   bytes, evaluation path, installed capability version and failure authority.
   `DomainGenerationService` owns configured source snapshots and rejection of
   configured Domain-only calls. `CppCoreGenerator.main` owns the default offline,
   qualification-flag and explicit compiled-model branches. Keep those contracts
   distinct from Spring startup assembly and template preview.
2. Select one uncovered supported configuration at a named entry. Reproduce its
   actual locked-original/current outcome before changing routing or metadata
   interpretation. Reuse the existing compiler service and Context/runtime
   mechanics, preserve unconfigured callers until their replacement is qualified,
   and avoid silently installing a new tool requirement or falling back after a
   configured compiler refusal. Carry changed metadata into a real generated
   consumer when it affects query, Checker/Fix, schema, Save or safe publication.
3. Reconcile the remaining configuration and protected-consumer rows against
   E01/E02 and the existing SQ/GP requirements. E03/E04 require their own explicit
   dispositions. Once implementation settles, reproduce affected artifacts and
   run the final current native/module/compiler integration joins required by
   P7. Routine reader repairs use valid and invalid inputs in both Python modes;
   they do not repeat completed business execution.

Each delivery must preserve the independent runtime/service/executor/provider
layers, two Context lifetimes, generated camelCase application APIs and the
SQLite-only edge scope. The goal remains active until every original P0-P7 exit
is satisfied; this routing checkpoint does not change its objective.
