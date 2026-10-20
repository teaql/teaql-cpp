# Generated Business-ID environment startup

Status: R04 generated startup responsibility qualified by the
[terminal checkpoint](parity/business_environment_adapter_terminal_5_0_6.json.gz).
This is the R04 responsibility in the
[G12 source review](g12_source_responsibility_review_5_0_6.md), not full G12/P6
or P0-P7 acceptance. References remain Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Public generated contract

All three source-shaped entries return
`Result<Ref<DataServiceExecutor>, ServiceRuntimeError>`:

- `serviceRuntimeFromConnection(user_context, connection)` accepts a checked
  `DataServiceConnection`, including an explicitly rebound Global connection.
- `serviceRuntime(user_context, config)` opens the configured SQLite connection
  before delegating to the connection entry.
- `serviceRuntimeFromEnv(user_context)` reads the modeled database URL before
  delegating to configuration assembly.

Applications check the Result before initializing schema or executing business
operations. This repairs the partially emitted connection entry's previous
infallible `Ref` return. Existing successful handwritten callers now check it.
Foreign or expired C++ bindings still raise their established binding exceptions;
ordinary allocation exceptions are not converted into configuration failures.

Models with Business IDs also expose the authentic public constants:

```cpp
BUSINESS_ID_KEY_HEX_ENV     // "TEAQL_BUSINESS_ID_KEY_V1_HEX"
BUSINESS_ID_KEY_VERSION_ENV // "TEAQL_BUSINESS_ID_KEY_VERSION"
```

Their connection adapter supplies
`install_generated_business_id_environment_key` to the qualified shared runtime
initializer. Module, metadata, SQLite provider, internal IDs and Business-ID
infrastructure precede key setup. Successful setup precedes executor construction
and registration. Ordinary models omit the constants and initializer and do not
read a key environment variable. Both model shapes retain explicit schema setup.

`configureRuntime(user_context, filename)` and `setup(user_context, filename)`
retain their existing direct-filename/manual-key contract. They do not start
reading environment keys. Application-owned typed providers remain supported
through that path. The generated Assist distinguishes these paths and documents
the typed Result, ownership and failure ordering.

All providers and nested errors belong to the supplied UserContext. Results are
checked borrows rather than owners of that Context. Failed startup leaves its
already installed infrastructure for actual Context cleanup; no replacement
executor is registered. Close a failed Context before business execution.

## Runtime-first implementation and preservation

The [key/environment kernel](business_key_environment_kernel_5_0_6.md) and
[pre-executor initialization](business_runtime_initialization_5_0_6.md) were
qualified before modifying the producer. Generated bodies contain only shared
assembly calls and existing service-error conversion. They do not reproduce
hex/version parsing, key encoding or provider assembly algorithms.

The [complete before snapshot](parity/business_environment_adapter_before_5_0_6.json.gz)
retains all 77 SDK file sets and producer/caller bodies. The
[producer scope proof](parity/business_environment_adapter_producer_scope_5_0_6.json.gz)
restores every previous CppCoreGenerator byte after removing only the reviewed
adapter region and two Assist paragraphs. Shared field/action Assist producers
and both frozen references remain unchanged. The shared support template gains
one runtime include.

`scripts/verify_business_environment_adapters_5_0_6.py` validates the complete
new delegate bodies, restores exactly the two previous runtime functions and
removes only the two conditional constants and their formatter separator. Every
other generated body/data/workflow must remain byte-identical. Reproduction uses
the actual Java producer rather than patching generated artifacts.

The [new Java regression](parity/business_environment_adapter_java_initial_failure_5_0_6.json.gz)
fails on the previous missing constants. The
[complete Java selection](parity/business_environment_adapter_java_all_terminal_5_0_6.json.gz)
passes 75 tests after the implementation, including ordinary-model absence and
manual-path preservation. An initial preservation-verifier failure retained the
formatter separator after removing constants; its
[record](parity/business_environment_preservation_initial_separator_failure_5_0_6.json.gz)
documents the verifier correction without changing production output.

The [caller audit](parity/business_environment_adapter_caller_audit_v2_5_0_6.json.gz)
checks sixteen previously discarded Results in fifteen handwritten observers.
The preservation verifier reconstructs those exact wrappers from the before
bodies and requires every other caller token to match. Existing checked calls,
returned Results and type-only compiler contracts stay intact.

Two compiler failures are retained. The
[inherited fixture](parity/business_environment_inherited_fixture_initial_compile_failure_5_0_6.json.gz)
used an unqualified `DataServiceExecutor` name that conflicted with the existing
runtime interface. It now names `cpp_inheritance_business_ids::DataServiceExecutor`.
The [initial caller audit](parity/business_environment_caller_audit_initial_compile_failure_5_0_6.json.gz)
wrapped two already checked multiline calls twice; the compiler correctly rejected
the resulting Ref where a Result was required. The corrected audit balances the
primary call and changes only a standalone expression statement. Both failed
build selections remain failed evidence; no production carrier or generated
alias was weakened to accept these fixture errors.

The [compiler controls](parity/business_environment_adapter_compiler_terminal_5_0_6.json.gz)
pass two positive and four negative cases: typed startup/ordinary configuration,
unchecked Ref conversion, ordinary-domain key constants, raw connections and
wrong configuration.

## Actual original startup observations

The unchanged original generated SDK supplies the 27 successful runtime scenarios
and eighteen startup errors in the
[original configuration record](parity/business_runtime_configuration_original_5_0_6.json).
The [fresh original boundary record](parity/business_environment_source_boundaries_5_0_6.json.gz)
adds seven boundary inputs through all three actual entries: 21 observations.
The binary hash matches the previously authenticated unchanged original observer;
all SDK/model/provenance/Cargo identities are checked before use.

The [original filesystem record](parity/business_environment_source_filesystem_5_0_6.json.gz)
adds two real environment-entry failures. Missing and invalid keys fail after
URL preparation creates the relative parent directory and database file, while
schema remains absent. The verifier observes relative paths, pre-inspection file
sizes, table names and the full typed startup error. Temporary paths are not
substituted into error text or returned JSON.

The C++ observer calls actual generated entries without manual key injection,
then executes schema, audited Receipt Save and typed readback. Error paths require
modeled Business-ID infrastructure and no registered application executor. Each
round actually deletes its UserContext and requires zero live User bytes.
Interrupted startup also verifies zero live User bytes after unwinding.

The full comparison has 68 scenarios: 27 successful configurations, eighteen
existing errors, 21 additional boundaries and two filesystem failures. It checks
complete JSON outcomes, not selected IDs or rewritten diagnostics. Three Unicode
slice cases preserve the original Rust panic versus C++ `std::out_of_range`
disposition with their distinct process exits and diagnostics. They are not
normalized into typed configuration errors or counted as identical JSON results.

## Terminal qualification and boundaries

The real producer strictly reproduces all 77 destinations: 63 successful SDKs
and fourteen preserved rejected-model outcomes. Exactly two domains emit the
conditional key adapter/constants, including the inherited Business-ID model.
The [reproduction record](parity/business_environment_adapter_regeneration_5_0_6.json.gz)
retains every artifact hash and command. No original Rust or historical C++
fixture is rewritten.

The final selected native build completes 21 relevant targets. Its first cohort
passes 21/22 checks; the remaining predicate comparator stops at its strict
observer identity guard before executing the semantic corpus. That guard
previously allowed only a fixture-path replacement. It now additionally requires
exactly the approved startup Result wrapper, while retaining byte equality for
every other C++ observer byte and the unchanged Rust observer. The failed cohort
remains in [its record](parity/business_environment_adapter_native_execution_v2_5_0_6.json.gz).
The unchanged binary then passes the complete isolated predicate comparison in
13.05 seconds. The terminal checkpoint preserves this follow-up; it does not
relabel the initial cohort as uninterrupted green.

Imported modules pass 7/7 and selected ASan/UBSan passes 4/4, with leak scanning
disabled. Both execute the same inherited fixture's five ownership and six
startup contracts. Separate complete
[native](parity/business_environment_adapter_native_observations_5_0_6.json.gz),
[module](parity/business_environment_adapter_module_observations_5_0_6.json.gz) and
[instrumented](parity/business_environment_adapter_sanitize_observations_5_0_6.json.gz)
records each retain all 68 actual C++ outcomes: 65 exact complete JSON matches and
three explicit panic/exception dispositions. These cover 66 successful governed
saves across the source scenarios and the required typed failures/file effects.
Sanitizer diagnostics are rejected even on the expected exception-exit paths.

Pinned formatting/checking covers 1,649 handwritten files. The baseline,
1498-export, 793-consumer and 75-runtime-root registration checks remain
mechanical evidence separate from these executions. Wider G12 assembly/sample
integration, the other partial capabilities and P7 remain active. Closing R04
does not establish complete runtime/generator parity.
