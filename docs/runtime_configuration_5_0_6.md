# Generated runtime configuration responsibility

Status: scoped configuration, typed errors, connection preparation and assembly
support are registered in production targets and module exports. Generated
configuration/env/checked-connection adapters and model-aware guidance are
implemented and reproduced across 24 workspaces and 44 contract fixtures.
The selected native 16/16, module 12/12 and ASan/UBSan 15/15 checkpoint passes
configuration, startup/query, error and lifetime controls. The complete compiler
checkpoint passes 41 positive/100 negative cases; later shared filename binding
passes its focused compiler/native/module/lifetime follow-up. Broader runtime
assembly qualification is in progress. G12/P6/P7 remain open.
References are Rust 5.0.6 and generator 45e, as locked by the project.
The [complete source anchors](parity/runtime_configuration_source_review_5_0_6.json)
retain ten configuration, assembly, connection and naming bodies, together with
the existing generated C++ runtime hash confirming the missing configuration type.

## Source behavior and current gap

The unchanged generated runtime has a distinct `ServiceRuntimeConfig`, a
`ServiceRuntimeError`, `service_runtime_from_env`, `service_runtime` and
`service_runtime_from_pool`. The configuration reads the model-derived
`DATABASE_URL_ENV`. Creating the runtime installs metadata, Checker/Behavior,
SQLite ID infrastructure, the provider and the separate application executor.
It deliberately leaves schema initialization to the application.

C++ already provides Context-owned assembly through `configureRuntime`, and
`setup` additionally initializes schema. Its filename parameter is passed
directly to `SqliteConnection`; it historically supplied neither the generated
environment/configuration entry nor source URL-to-path preparation. The new
source-shaped entries now supply them. These
are separate responsibilities from the already implemented provider/executor
stack. Do not change existing filename calls into URL calls implicitly.

Business-ID key installation has an existing explicit C++ ownership adaptation:
the application installs its key provider through UserContext. See
[Business IDs](generated_business_ids_5_0_6.md). Adding database configuration
must preserve that behavior and the original typed key/profile contracts.

## Actual unchanged original observations

The [source record](parity/runtime_configuration_original_observations_5_0_6.json)
contains twenty isolated subprocess observations. The observer calls the retained
generated public configuration/runtime functions, checks their actual errors and
performs an actual generated count after successful assembly. The test process
supplies synthetic environment values and a temporary working directory; it
does not modify the agent's environment or implement connection algorithms.

The record establishes these concrete source behaviors:

- Missing variables and non-UTF-8 values fail configuration with distinct
  original environment-error payloads. Empty and blank values are accepted by
  configuration; validation is not silently added there.
- Config clone/equality and Debug retain the supplied URL, including query text.
- The connection function removes `sqlite:`, then repeated **pairs** of leading
  slashes using `trim_start_matches("//")`, then takes the part before `?`.
  This is not general URI decoding or removal of every leading slash. The
  recorded three-slash case retains an absolute `/:memory:` path and fails in
  the observation environment; it is not another in-memory connection spelling.
- Parent directories are created. A query such as `?mode=ro` is stripped rather
  than enforced as a database open mode. Percent escapes remain literal. A
  `file:` URI also loses its query before the provider opens it.
- Successful assembly leaves tables absent: the actual generated count fails
  with the original missing `person_data` error. Filesystem creation and that
  query failure are recorded independently.
- A parent that is a regular file produces the original ConnectionError. The
  source's environment error, connection error and runtime query error remain
  distinct; no observer collapses them into a generic success/failure flag.

All retained generated files and their model hash are checked before observation.
Registry versions/checksums match the runtime reference lock. The independent
original build completed successfully before its executable was used. No C++
outcome is claimed by this source-only record.

## Implementation and acceptance requirements

1. Put reusable scoped configuration/error and SQLite path preparation mechanics
   in runtime/provider support, then emit thin domain adapters. Preserve separate
   transport, provider, runtime and application executor responsibilities.
2. Expose camelCase application configuration/environment APIs with the authentic
   model environment constant. Require UserContext for dynamic configuration and
   error ownership; copy nested strings into its allocator. Retention requires an
   explicit destination copy. No cached process environment or implicit Context
   owner is permitted.
3. Keep `configureRuntime(user_context, filename)` and `setup` compatible. Add an
   explicit configuration entry; keep schema initialization explicit for the
   source-shaped assembly entry. Reuse existing assembly instead of emitting a
   second provider setup algorithm.
4. Compare configuration, URL preparation, errors, resulting files and real
   generated query behavior with the recorded original observations. Qualify the
   C++ error-carrier/ownership adaptation separately. Test actual User deletion,
   explicit Global copies and null default PMR allocation.
5. Update model-aware Assist, compiler contracts and Java generation regressions;
   regenerate/reproduce artifacts and run relevant native/module/lifetime gates.
   This requirement cannot be closed by an environment constant or method name
   alone.

## Original reproduction

```bash
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_runtime_configuration_original_506 \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_execution_shapes_5_0_6/Cargo.toml \
  --bin runtime_configuration_original_5_0_6
python3 scripts/observe_runtime_configuration_original_5_0_6.py \
  --rust /tmp/teaql_cpp_runtime_configuration_original_506/debug/runtime_configuration_original_5_0_6 \
  --output /tmp/teaql_runtime_configuration_original_replay.json
```

The recording helper refuses to overwrite existing evidence. The connection
failure text can depend on the host's filesystem permissions and OS; compare
the actual source environment and C++ environment together before qualification.
Use `--check --output docs/parity/runtime_configuration_original_observations_5_0_6.json`
to replay the retained evidence without changing it.

## Shared kernel implementation in progress

[generated_runtime_config.hpp](../teaql_runtime/include/teaql/runtime/generated_runtime_config.hpp)
and its implementation provide explicit-domain configuration values, separate
NotPresent/NotUnicode environment errors and distinct MissingEnv, ConnectionError
and Configuration preparation errors. Nested bytes and diagnostic strings copy
through their allocator-aware constructors. Environment reads are immediate;
no cached process state owns configuration data. The full generated service error
must additionally preserve its provider/runtime branches when adapters are added.

The independent SQLite [path helper](../teaql_provider_sqlite/src/runtime_config.cpp)
retains source pair-slash trimming, query removal and parent creation without
changing the existing native filename API. Both implementation files pass current
toolchain syntax checks. The new ownership/path fixture also compiles after its
initial incorrect Ref access was fixed to resolve through UserContext; that
[initial failure](parity/runtime_config_initial_ref_access_failure_5_0_6.json.gz)
is retained. No linked runtime execution, lifetime or original/C++ comparison is
claimed yet.

Two further [direct-config source observations](parity/runtime_configuration_nul_original_observations_5_0_6.json)
establish embedded-NUL ordering. A NUL in the parent directory fails without
creating it; a NUL in the filename creates the valid parent before connection
validation fails with its byte position. Preserve both errors and the filesystem
boundary before using a C++ filesystem/native-string interface that could
truncate those bytes. Both guards have now been added and the updated provider
source passes its syntax check; the initial syntax record does not claim linked
original/C++ execution.

The [subsequent standalone execution checkpoint](parity/runtime_configuration_kernel_execution_5_0_6.json)
now passes native and selected ASan/UBSan ownership/path execution, with null
default PMR, actual User deletion, expired reference rejection and explicit
Global copies. The kernel probe matches all seven retained configuration outcomes
and both NUL errors/created-path observations against live unchanged original
executables. A deliberately wrong probe proves comparator rejection.

Read-only no-work checks confirmed the five required dependency archives were
already complete in each profile. Independent compilers/linkers wrote into
separate `/tmp` directories and exited successfully before execution; archive
hashes remained unchanged. This permits the focused kernel gate while unrelated
native integration targets continue compiling. Leak scanning is disabled.
Ordinary successful connection/startup and the full five-branch service error
composition are not qualified by these nine kernel outcomes.

The [complete service error carrier](../teaql_runtime/include/teaql/runtime/generated_service_runtime_error.hpp)
now retains all five original alternatives through a provider-neutral template.
MissingEnv/ConnectionError/Configuration delegate to preparation errors;
Rusqlite and Runtime retain their concrete typed payloads. Explicit source-display
and source-Debug access copy into a chosen destination, while typed payload
access remains borrowed. No provider or runtime error is flattened into the
preparation message string.

All [five original public branches and From conversions](parity/runtime_service_errors_original_observations_5_0_6.json)
match the native and selected ASan/UBSan carrier executions exactly, including
Display, Debug, source and source-Debug. The ownership test copies each branch
into Global, actually deletes UserContext, rejects all five old references and
checks concrete provider/runtime destination ownership with null default PMR.
See the [terminal evidence](parity/runtime_service_errors_execution_5_0_6.json).
This closes the demonstrated missing error composition in shared support;
generated assembly, production/module registration and wider configurations
remain required. Leak scanning is disabled.

The [Context-owned connection entry](../teaql_provider_sqlite/src/runtime_connection.cpp)
now opens a real SQLite resource after URL preparation. All thirteen original
connection outcomes and created-file observations match standalone native and
selected ASan/UBSan executions. The comparison explicitly excludes only the
successful source's subsequent generated query error; metadata/executor/query
assembly is a separate required boundary. It does not normalize error text.

The [initial two discrepancies](parity/runtime_connection_initial_comparison_5_0_6.json)
led to a source-reviewed repair: use the original ancestor mkdir order instead
of the standard create_directories error for an existing ordinary file, and
retain the source CannotOpen filename context over the unchanged filename API.
The cleanup observer checks both a 32 KiB host buffer and its still-alive object
flag during native SQLite close. Actual User deletion closes once, destroys that
host afterward, returns all User blocks and invalidates the connection ID.
See the [terminal connection evidence](parity/runtime_connection_execution_5_0_6.json).
The provider and runtime layers remain separate and no implicit owner is added.

The [shared generated service assembler](../teaql_provider_sqlite/include/teaql/provider_sqlite/runtime_assembly.hpp)
now applies the selected domain module and configures the existing transport,
provider, internal IDs, optional Business-ID infrastructure and distinct domain
executor through their existing runtime contracts. It accepts a checked
connection ID, resolves it through UserContext and requires the module's
explicit destination domain. It does not create schema or install an implicit
Business-ID key provider.

The [complete startup comparison](parity/runtime_assembly_config_execution_5_0_6.json)
passes thirteen full original/native/selected-SAN observations. The actual
retained generated module and executor participate, and the original successful
post-startup generated query error is now compared rather than projected away.
Files, errors and values remain exact. Null default PMR and actual User deletion
pass; leak scanning remains disabled. Public generated configuration methods,
production/module registration and their Java/Assist/reproduction controls remain
required.

Pinned format/apply/check now passes for 1,465 files. The formatter preserves
all 1,460 existing C++ files' bytes and timestamps; only the five new files were
added. See the [syntax/format checkpoint](parity/runtime_configuration_kernel_syntax_5_0_6.json).
The existing native integration build now completes all 1,067 steps with exit
zero. Its full 658-test native CTest run is still in progress and has reported
an initial bootstrap-extensions timeout at 180 seconds. Successful compilation
does not establish a successful integration gate; preserve the complete test
result and diagnose failures before acceptance.
The new files have not been added to their CMake/module inputs, so earlier
compiled interfaces and generated artifacts remain unchanged. Registration,
complete service error composition, thin adapters, regeneration, Assist and
native/module/lifetime/source-comparison checks remain required.

## Production registration and generated adapters

The [production registration checkpoint](parity/runtime_configuration_production_registration_5_0_6.json.gz)
records terminal native and imported-module builds, six checks in each frontend,
and 58 passing Java generation tests. Configuration/path storage, complete error
carriers, native cleanup and full source startup/query observations now link the
actual production archives. The production runtime remains provider-neutral;
the first fixture registration incorrectly linked its SQLite-path fixture to
runtime alone. Its [initial failure](parity/runtime_configuration_registration_initial_failure_5_0_6.json.gz)
is retained, and the fixture now explicitly links the SQLite package.

The producer adds `ServiceRuntimeConfig::fromEnv(user_context)`, `databaseUrl()`,
`serviceRuntimeFromEnv`, `serviceRuntime` and `serviceRuntimeFromConnection`.
The model environment constant is obtained from the original centralized naming
function, including explicit model crate names. The checked connection entry
reuses shared assembly; errors preserve concrete preparation/provider/runtime
payloads. Global connections require explicit UserContext rebinding, and foreign
configuration requires an explicit destination copy. Existing direct filename
`configureRuntime` and schema-initializing `setup` bodies are unchanged.

These generated APIs have not yet passed C++ execution acceptance. A controlled
reproduction mode permits only the reviewed new adapters, four support includes
and Assist, checks the environment constant against the unchanged Rust artifact,
and verifies every prior generated body remains byte-identical after pinned
formatting. Broader configuration, lifetime and complete G12/P6/P7 gates remain.

The previous complete compiler follow-up terminated with another stale negative
expectation: `withRecordedAtIs("raw timestamp")` compiled, exactly as the original
`with_recorded_at_is(impl Into<Value>)` permits. The [failure record](parity/generated_compiler_timestamp_followup_5_0_6.json.gz)
is preserved. One corrected positive query case and two genuine negative
setter/unsupported-value cases pass; this is not a full compiler-suite result.

## Generated artifact and ownership follow-up

The [artifact checkpoint](parity/runtime_configuration_adapter_artifacts_5_0_6.json.gz)
records 24 regenerated/reproduced workspaces (288 files), all 44 reproduced
contract fixtures, and two positive/six negative compiler controls. Thirty
contract fixtures emit SDKs with eight reviewed new declarations each; fourteen
rejected-model fixtures preserve their exact prior error observations. All old
generated functions remain unchanged. The initial observer-only fixture lookup
[failure](parity/runtime_configuration_contract_reproduction_initial_failure_5_0_6.json.gz)
is retained; environment-source lookup is now limited to actual runtime outputs,
without skipping any rejected-model observation bytes.

The first generated native selection passed the nine configuration/NUL and
thirteen complete startup/query source comparisons, plus the existing Q/E
example. Its ownership test exposed an actual provider gap: User-owned provider
state rejected an explicitly rebound Global connection. The [initial failure
and exact provider source](parity/runtime_configuration_generated_initial_failure_5_0_6.json.gz)
are preserved. This failure is not waived by the source-only startup successes.

The repair permits connection ownership to outlive provider ownership while
validating both separately. A User-local provider on a Global connection must
reject work retained by another User or Global. Provider clones cannot retain
User layouts in Global, and Global providers cannot retain User connections.
A new native fixture opens a real cursor, queues work through separate User-local
providers, destroys a queued Context and then the cursor's Context, and checks
that the surviving query is awakened, the cancelled write never ran, no statement
is busy and all User bytes are returned. The generated fixture checks explicit
configuration relocation, executor/config expiration, schema initialization and
a real audited row surviving through a correctly rebound Global connection.
Native/module/instrumented execution of the repair remains pending. SQLite
resources retain the existing owner-thread confinement; this does not claim
thread-safe shared connection execution.

The first Global-connection repair selection passes native 16/16, modules 12/12
and selected ASan/UBSan 15/15. The [intermediate checkpoint](parity/runtime_configuration_global_connection_first_followup_5_0_6.json.gz)
remains separate from final acceptance: review caught that the first patch
unnecessarily rejected synchronous explicit destination copies. Those calls
complete without retaining provider state, so their existing checks are restored.
A dedicated regression copies real SQLite rows into Global, closes every User,
and reads the copied value afterward. The stronger lifetime restrictions apply
to retained asynchronous work and dependency construction, without reducing
synchronous destination-copy behavior. The complete compiler gate remains live;
final repaired execution follow-ups are pending.

## Current execution checkpoint

The [final selected execution checkpoint](parity/runtime_configuration_generated_execution_5_0_6.json.gz)
passes native 16/16, imported modules 12/12 and ASan/UBSan 15/15 after restoring
synchronous explicit-copy behavior. Pinned formatting applies/checks all 1,476
handwritten C++ files. Every compiler/linker terminated successfully before its
output ran. Source comparisons retain all nine configuration/NUL, thirteen
startup/query and five complete error branches. Actual User deletion, explicit
Global copies and connection rebinding, cancelled queued writes, cursor release,
no busy statement and zero remaining User bytes pass with null default PMR.
Leak scanning remains disabled.

These are selected responsibility gates. The previous full native integration
remains a historical 654/658 result with four failures and their separate
repairs. Complete generated compiler validation is still active, and broader
model/Business-ID/runtime configuration, registration/customization, sample data,
Assist and final integration requirements remain in the full G12/P6/P7 plan.
No full runtime or generator parity is inferred from this checkpoint.

The [source/consumer integrity follow-up](parity/runtime_configuration_source_integrity_followup_5_0_6.json)
verifies 684 canonical imported bodies, 1,493 exports across seven modules, the
actual import/Context/template/ownership/ABI executable, locked source integrity
and active package paths. These checks identify registration/source consistency;
they do not replace the executed comparisons or close the eleven remaining
capabilities. The full generated compiler worker remains live at this checkpoint.

The [complete compiler checkpoint](parity/generated_full_compiler_configuration_terminal_5_0_6.json.gz)
now passes all 41 positive and 100 negative generated API cases. It records its
actual header/script inputs and precedes the subsequent shared filename-binding
refactor. It supersedes the earlier pending status without relabeling those
records. [Business-ID assembly](business_runtime_configuration_5_0_6.md) further
qualifies 27 original/C++ scenarios and 54 actual Context rounds through all three
source entries and the documented typed-key ownership adaptation. Complete G12
configuration/customization and P6/P7 remain open.

Shared filename delegation and Business-ID configuration now have their final
[generated execution checkpoint](parity/runtime_binding_final_execution_5_0_6.json.gz):
native 12/12, modules 8/8, ASan/UBSan 8/8 and focused compiler 4-positive/7-negative,
with exact literal filename/failure-order behavior, reproduced artifacts and
27 original/C++ Business-ID scenarios across 54 actual Context rounds. The
[runtime assembly matrix](runtime_assembly_qualification_matrix_5_0_6.md) preserves
the remaining standalone registries, schema-provider/type surfaces and wider
configuration/customization/sample/Assist requirements. G12 remains partial.
