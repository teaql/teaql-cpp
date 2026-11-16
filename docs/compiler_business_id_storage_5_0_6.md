# Compiler-resolved Business ID storage

## Confirmed gap

The [actual v3 controller diagnosis](parity/compiler_business_id_storage_diagnosis_5_0_6.json.gz)
uses the unchanged `tests/generator/business_ids.xml` model. Public evaluation,
complete original generation and original compilation succeed. MIR, complete
descriptor observation and scalar observation also succeed. C++ generation then
exits 1 with `UNMAPPED_TYPE` for `receipt.receipt_number`; no C++ SDK is published.

The original getter returns the generated strong `ReceiptNumber` type; Ticket's
getter similarly returns `TicketNumber`. The v3 scalar observer recognizes
ordinary TeaQL scalar identities, so it records both wrappers as `unmapped:`.
The C++ producer compares that fact with its String storage carrier and refuses.
Existing C++ Business ID types and runtime services already supply the intended
abstraction. This is an incomplete compiler mapping, not a Rust reference defect
or an additional compatibility exception.

The original wrapper has a public string view and `From<&Wrapper> for Value`;
its underlying entity property is String. The canonical constructor is private
to the original crate and must not become an observer dependency. Wrapper names
must come from the locked `RustFieldDescriptorNaming.rustBusinessIdTypeName`
helper and owning module, preserving distinct strong identities. The observer
must use the public module re-export, since the `entity` submodule is private.

## Implementation plan

1. Add an explicit, versioned semantic-storage observation. Preserve the existing
   capability/document formats and distinguish actual wrapper identity, semantic
   Business ID responsibility and its String representation. Do not relabel the
   wrapper as a primitive or serialize process-specific TypeId numbers.
2. Derive the wrapper identity from authentic locked metadata/naming helpers.
   Bind the getter's TypeId to that generated wrapper in the same original
   library, with compiler-checked public string/value conversion contracts.
   An arbitrary unknown type with a similar method must remain unmapped.
3. Carry complete source-owned facts through the worker and generation-scoped
   resolver. Validate model/library/field membership and strict protocol members.
   Keep C++ String storage and existing strong Business ID application APIs;
   preserve the independent allocator, profile, business clock and Context SPIs.
4. Verify complete original/C++ generation and actual typed Business ID reads,
   Q/E/JSON and relevant allocation/persistence consumers. Include prefix,
   namespace, profile, inheritance and invalid witness/type/membership controls.
   Retain explicit original compiler refusals and unrelated unsupported types.
5. Run current Java/worker contracts, retained SDK reproduction and affected
   native checks. Qualify ordinary/optimized validators and exact source epochs,
   remove owned disposable outputs, record English evidence and publish.

The successful diagnostic removes its parent and worker outputs after recording
raw commands, compiler output, scalar facts and the exact C++ failure. It adds
no C++ runtime dependency or ownership domain. The pending Option-model and
column-attribute choices, remaining configuration mappings, G13/G14/G17 and
all outstanding P0-P7 exits retain their full scope.

## Implementation and executed consumers

The producer working tree adds an explicit semantic-storage capability
(`job.v4`, `model.v3`, `model_generation.v3`). It retains the preceding explicit
formats and separates the original strong getter identity from its String
storage carrier. The generated observer compares the getter's TypeId with the
authentic source-named wrapper in the same library and requires the public
string view and `From<&Wrapper> for Value` contracts. It neither calls a getter
nor depends on a private canonical constructor. The generation-scoped resolver
checks complete field membership and semantic/storage responsibility before
returning the existing String carrier to the C++ emitter.

The [execution receipt](parity/compiler_business_id_storage_execution_5_0_6.json.gz)
contains these actual outcomes:

- The unchanged regular Business ID model completes the public controller,
  original compilation, descriptor/configuration/semantic observation and C++
  generation. Three entities supply thirteen scalar facts. ReceiptNumber and
  TicketNumber retain their actual, distinct Rust types with String storage.
  All 25 original SDK files reproduce identically in an independent generation;
  the candidate publishes a complete 53-file C++ SDK.
- All 1,044 original/C++ Checker observations agree with the existing declared
  resource type-name presentation. They include the actual prefix, namespace,
  profile, digit count and separator definitions, loading states, registered
  services, active roots and governed validation/repair results.
- All 96 strong E observations agree, including NULL, NotLoaded, typed values
  and fallback behavior. The existing diagnostic presentation adapter is reused.
- All 480 Receipt/Ticket typed Q lookup observations agree exactly, including
  actual SQLite execution, intent, selected rows, JSON and SQL diagnostics.
- The unchanged native allocation/Save test passes against the new SDK. It
  checks distinct strong types, Unicode trimming, explicit Global retention
  after UserContext destruction, shared changes, real Business ID allocation,
  typed queries and rejection of a dirty identifier during governed Save.
  This last test is native behavior evidence, not an additional paired Rust
  Save execution.

Only the regular generated-header include is redirected in each copied C++
observer; business bodies are unchanged. The Checker observer also includes
the retained relation header and the lookup observer includes the retained
inheritance header so their existing branches still compile. This checkpoint
executes no relation branch or Document lookup from those dependencies. The
Rust observer bodies and the copied Checker helper source are unchanged.

## Original inheritance boundary

The unchanged inheritance model reaches the actual original compiler and
reproduces exactly the twenty code/message pairs in the
[existing failure record](parity/inherited_business_source_failure_5_0_6.json).
The original descendants still lack DocumentNumber and their Business ID slots.
The controller returns the explicit compiler refusal; no candidate SDK or
semantic-storage observation is published for that model. Its independently
generated original SDK remains byte-identical to the worker's original output.

The [previously qualified adapted-source bridge](inherited_business_binding_bridge_5_0_6.md)
retains its separate scope. This run adds no untouched whole-crate inheritance
acceptance, new compatibility exception or capability promotion.

## Contracts, admission and next exit

Worker contracts pass 18 tests under ordinary Python and 18 under optimized
Python. Fresh Java compilation and all 265 generator contracts pass. The raw
worker outcomes are retained in
[`compiler_business_id_storage_units_current_5_0_6.json.gz`](parity/compiler_business_id_storage_units_current_5_0_6.json.gz);
that receipt also retains a Java launch failure caused by the wrong working
directory. The corrected Java run is separately retained in
[`compiler_business_id_storage_units_java_5_0_6.json.gz`](parity/compiler_business_id_storage_units_java_5_0_6.json.gz).
The earlier `units_candidate` receipt retains an import-path launch failure.
All three owned unit temporary directories have been removed. The first two
consumer attempts retain their separate failures: the default Rust linker
could not resolve SQLite's fcntl64; the independent native lookup build then
lacked the CMake-provided source-directory macro. The successful business
commands use the existing locked linker and that same source-directory macro.
No observer body or business implementation was changed to repair these
launchers. Their disposable outputs were removed after compact logs were saved.

All compiler/business commands in the final execution receipt finish with their
expected outcomes. That invocation subsequently exits 1 in its original final
reader: the controller reserializes the source-package envelope, whereas the
independent generator receives Python's JSON encoding. The original reader
wrongly demanded equal hashes for these different JSON byte encodings. Both
packages carry exactly the same complete base64 model bytes and produce
byte-identical complete original SDKs.

The separate [admission record](parity/compiler_business_id_storage_admission_5_0_6.json.gz)
retains that failure and the precise source continuation. The corrected reader
checks each envelope's actual hash, the identical source-package data and the
complete SDK manifests. It binds the earlier execution source by exact hash
and AST equality of every execution/comparison/command function; only the
reader changes and one source-identity helper is added. This does not claim a
second business execution or relabel the original exit status. Ordinary and
optimized Python each accept the complete receipt and reject all 33 recorded
corruptions, including foreign wrappers, primitive substitution, altered
library identity, lost results, wrong raw input/output, fabricated Save output
and damaged original refusal evidence.

The execution frame contains 12,094 inputs: the preceding 12,093 remain present,
five expected producer implementation/test inputs change and one verification
script is added. All 1,820 existing C++ inputs retain their hashes. Temporary
compiler, consumer and admission directories are owned and removed; shared
caches are untouched. No sanitizer, MIPS or physical-router execution is added.

## Retained SDKs and current source qualification

The [SDK reproduction](parity/compiler_business_id_storage_sdks_5_0_6.json.gz)
completes all six refresh/check actions and 308 actual subprocess invocations.
All eighty retained SDKs preserve all 3,560 generated code and Assist files.
Each manifest updates only the existing CppCompilerModelConfiguration source
fingerprint. The three generation recipes retain their exact preceding ASTs
and all checks execute unconditionally. Generated files are regenerated through
the recipes rather than edited manually.

The [source admission module](../scripts/compiler_business_id_source_admission_5_0_6.py)
joins immutable runtime, SDK, unit and source-image evidence. It registers the
five producer changes, both actual execution drivers, the earlier envelope
reader repair, five narrowly bounded reader continuations and eighty manifest
transitions. Exact before/after hashes and function ASTs preserve execution and
comparison bodies. The preceding source graph remains intact; unknown images,
missing identities and unregistered Java additions reject. Historical SDK
records may use only source fingerprints connected to the installed producer
by this explicit graph; all other manifest members and generated bodies remain
exact. SDK validator configuration is restored after each composed review.

The first source-admission attempt assumed the earlier failed combined unit
launcher had emitted its final input map. That reader failure is retained in
`compiler_business_id_storage_source_admission_initial_5_0_6.json.gz`. A fresh
[standalone worker run](parity/compiler_business_id_storage_worker_units_5_0_6.json.gz)
records both input frames and passes eighteen tests per Python mode. The
successful 265-test Java run and earlier failures retain their separate scope.

The [compatibility record](parity/compiler_business_id_storage_compatibility_5_0_6.json.gz)
passes the complete positive receipt checks and these corruptions under both
ordinary and optimized Python:

| Receipt/source responsibility | Rejected cases per Python mode |
| --- | ---: |
| Business ID source admission | 12 |
| Preceding scalar source admission | 13 |
| Business ID execution | 33 |
| Retained JSON consumers | 22 |
| Retained E consumers | 24 |
| Total | 104 |

These source/receipt checks do not add another original business execution.
The [affected native gate](parity/compiler_business_id_storage_native_5_0_6.json.gz)
passes eight tests with exact registry/JUnit membership and no failures,
errors or skips, including three required source fixtures. The selected native
build reports no work. The final pinned C++ formatter write/check and both Git
whitespace checks are retained in
[final checks](parity/compiler_business_id_storage_checks_5_0_6.json.gz).

This closes the bounded regular Business ID compiler-mapping continuation and
its SDK/source compatibility exit. Complete configuration mappings, the pending
Option-model and column-attribute decisions, G13/G14/G17 and every remaining
P0-P7 exit stay open. The existing inheritance adaptation retains its explicit
boundary. No compatibility exception or capability status is added: fourteen
capabilities remain qualified and three remain partial.
