# Source generation through the Rust compiler

The producer's normal generation service can now install a trusted source
compiler for `cpp-lib-core`. Once installed, source-based requests invoke the
actual Rust compiler job automatically. Callers provide their model files, not
precomputed projection facts or additional runtime/provider arguments.

This extends the [model-driven job](cpp_compiler_job_5_0_6.md) into the existing
service/controller path. It does not complete all Rust configuration routing.
The no-argument service still supplies the existing unconfigured generation path;
removal of redundant interpretation follows the
[compiler resolver plan](rust_configuration_compiler_resolver_plan.md).
G13/G14/G17 and the complete P0-P7 goal remain open.

## Trusted installation and ordinary calls

```java
var cpp_compiler = new CppCompilerGeneration(
    python_path, worker_path, capabilities_path, temporary_root,
    Duration.ofMinutes(10));
var generation_service = new DomainGenerationService(
    Map.of(CppCoreGenerator.SCOPE, cpp_compiler));

var generated_files = generation_service.generateDomainFiles(generation_input);
var controller = new GenerateController(generation_service);
```

`generation_input` is a `GenerateContext` with its entrance filename and complete
file map. Scope installation belongs to trusted host construction. Model
attributes, uploaded JSON and request parameters cannot install or replace a
compiler capability. The immutable service map and immutable tool configuration
can be reused by concurrent calls; source bytes, subprocesses and temporary
directories belong to each invocation.

The existing controller retains validation and ZIP delivery, then passes the
original input to `generateSourceFiles`. The configured service copies the
source map and every byte array before dispatch. Its old Domain-only API rejects
a configured compiler scope when source files are absent, rather than generating
through another path. Unconfigured scope behavior remains available and has
separate byte-preservation checks.

## Complete model input

`teaql.model_source_package.v1` transports exactly three members: `schema`,
`entrance_file` and `files`. File values are canonical Base64. This preserves
binary bytes, relative names and XML origins, including models assembled through
`_include`. Duplicate keys, missing entrance data, unknown package members and
ambiguous or unsafe source names reject.

The same `CppModelSource` source is compiled separately against original and C++
model libraries. Each parser receives all the original files. The locked Rust
generator and original generated Rust remain unchanged. `CppCompiledModel`
delegates to the existing C++ emitter and resolver; it does not replace their
model/query/graph implementations. In source-package mode, `model_sha256` binds
the actual package bytes, including filenames and the entrance, for both tools.

The compiler worker retains the actual package bytes in its compressed report.
The service pins its interpreter, worker and capabilities, checks report/input
bindings and verifies every returned SDK file against the complete compiler-job
manifest. It returns exact byte arrays, without reformatting or re-encoding them.

## Cleanup and failures

The service waits for the worker and applies a bounded deadline. Interruption
or timeout stops and reaps the owned worker before removing its directory.
The worker separately owns and closes its Rust/compiler and publication trees.
SDK bytes are returned only after the service directory closes. A trusted report
observer can receive a cloned compressed receipt after cleanup; there is no
mutable last-report cache.

`CompilerFailure` retains a code and cloned compressed report. Actual original
compiler refusal remains distinct from toolchain, protocol, capability, mapping,
timeout and cleanup failures. A refusal does not trigger the unconfigured
generator. Existing SDK paths are not used as an implicit result cache.

This is generation tooling. It introduces no Rust, Java, Python, web or other
dependency into the retained C++ SDK or edge runtime. GlobalContext/UserContext,
Q/E bindings and the data-service, SQL executor, transport and provider layers
keep their existing implementations and responsibilities.

## Executed acceptance

The [current service execution](parity/compiler_source_service_current_5_0_6.json.gz) freshly
compiles the original/current tools and runs 252 Java contracts. The original
source-package bridge also passes its two contracts; the worker's ten Python
contracts pass in both Python modes in the
[Java/tool receipt](parity/compiler_source_service_java_5_0_6.json.gz).
An earlier HTTP fixture failed the existing KSML rule requiring a business
object. The fixture was repaired; the validation rule was preserved and that
incomplete receipt remains separate. The regular Maven reproduction then exposed
an undeclared Spring mock dependency in the new test. The test now uses the
existing Servlet/MultipartFile interfaces and a JDK proxy; no dependency was
added. The earlier passing broad-classpath epoch and the Maven failure remain
historical diagnostics, separate from the current execution.

Four actual Rust model jobs then execute through the configured service: a
multi-file model, an actual compiler refusal, an entity-only model and the
existing controller's uploaded-ZIP route. Multi-file input contains relative
includes and a binary resource; all names and bytes survive both parsers. The
configured model also contains inheritance, an overridden compiled owner, a
function-pointer constant expression and an external Rust include.

Three successful SDKs actually compile and execute as C++. Their nine entity
descriptors match the original compiled descriptors. Default PMR is disabled;
UserContexts are destroyed with no live user bytes. Changing the Rust include
to a boolean produces E0308, returns `REFERENCE_COMPILER_REFUSAL` and returns no
SDK. Service/compiler/publication directories close before return/report delivery.

The controller acceptance invokes the actual controller, validator, input loader
and ZIP writer with mock HTTP objects; it does not start or deploy a server.
Fourteen receipt corruptions reject under
[ordinary Python](parity/compiler_source_service_controls_current_5_0_6.json.gz) and
[optimized Python](parity/compiler_source_service_controls_current_optimized_5_0_6.json.gz).
These checks cover source/package bindings, complete SDKs, native execution,
refusal behavior, cleanup and required process/Java-test membership.

The [default compiler-contract continuation](parity/compiler_source_service_preserved_contracts_5_0_6.json.gz)
freshly generates all 406 complete SDKs with identical bytes, explicitly reusing
their preceding C++ compiler outcomes. The
[retained SDK continuation](parity/compiler_source_service_sdks_current_5_0_6.json.gz)
regenerates and strictly checks all eighty SDKs. Their current manifests and all
3,560 code/Assist files remain unchanged. Each continuation rejects eight
corruptions per Python mode. These checks do not add business SQL, graph Save,
Checker/Policy or log-sink execution for the new configuration.

## Historical source continuation

The [source admission helper](../scripts/compiler_source_service_source_admission_5_0_6.py)
binds fourteen executed implementation/recipe paths to the current service
receipt. Exact, immutable transitions preserve the bodies of the historical
generation readers while forwarding only their source-version checks. A pure
hash graph connects consecutive helper versions without recursively validating
the same evidence. Unknown hashes and unregistered Java additions still reject;
the original corpus-specific before-only rules remain active.

The final [ordinary](parity/compiler_source_service_source_controls_verified_5_0_6.json.gz)
and [optimized](parity/compiler_source_service_source_controls_verified_optimized_5_0_6.json.gz)
checks each cover 34 corruptions/source epochs. The original compiler admission
also passes all 108 controls in
[ordinary](parity/compiler_source_service_legacy_source_controls_verified_5_0_6.json.gz)
and [optimized](parity/compiler_source_service_legacy_source_controls_verified_optimized_5_0_6.json.gz)
Python. The [plain XML continuation](parity/compiler_source_service_plain_jobs_5_0_6.json.gz)
executes four original compiler jobs and two generated C++ observers; its sixteen
receipt controls pass per Python mode.

The [affected native CTest receipt](parity/compiler_source_service_native_verified_5_0_6.json.gz)
records both reverse-relation and cross-context checks passing, 2/2 in each
Python mode. These tests revalidate retained SQLite evidence; they are not new
business SQL executions or a new full native epoch. The
[initial](parity/compiler_source_service_native_5_0_6.json.gz),
[intermediate](parity/compiler_source_service_native_current_5_0_6.json.gz) and
[preceding final attempt](parity/compiler_source_service_native_final_5_0_6.json.gz)
remain immutable diagnostics. They exposed missing historical source forwarding;
the preceding final attempt passed the reverse-relation check while still
rejecting the cross-context caller's old service hash.

The [final checks](parity/compiler_source_service_checks_5_0_6.json.gz) pass
formatting, locked-baseline, generator-ledger, module-path, no-work native build
and whitespace checks. The
[ordinary source review](parity/compiler_source_service_source_review_5_0_6.json.gz)
and [optimized source review](parity/compiler_source_service_source_review_optimized_5_0_6.json.gz)
admit twenty exact source/evidence changes on 12,035 inputs and reject four
substitutions each. All 1,818 preceding C++ implementation/fixture inputs and
707 native executable hashes remain unchanged. Ledger changes append evidence
only; every capability status is preserved.

The four successful current service/plain/default/SDK qualification receipts
record removal of 23,679,038 bytes of owned temporary outputs. Their directories
are absent. Shared compiler, Maven and Cargo caches remain in place. Compact
receipts retain source identities, commands, outcomes and earlier diagnostics.

## Next implementation sequence

Continue Stage 3 of the
[compiler resolver plan](rust_configuration_compiler_resolver_plan.md): inventory
the remaining Rust configuration consumers, obtain authoritative facts through
the locked parser/compiler path, and map those facts through existing C++ runtime
abstractions. Preserve explicit failures for valid Rust types without a faithful
C++ mapping. Pending source-behavior decisions keep their existing scope.

After that coverage is established, execute the generated SQLite Q/E/JSON,
graph Save, Checker/Policy and ordinary/debug sink gates for the new path.
Remove redundant interpretation only after those contracts are covered. Full
G13/G14/G17 and P0-P7 acceptance remain separate from this source-service checkpoint.
