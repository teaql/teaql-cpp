# Compiler-resolved scalar storage

The complete configuration resolver now uses compiler-observed scalar identity
when the caller selects the new explicit capability. Previously it still called
the unconfigured generator's finite Rust type spelling check and rejected valid
qualified scalar aliases. Descriptor `DataType` alone is insufficient: it does
not distinguish `f32` from `f64` or establish a custom type's C++ representation.

## Plan

1. Reproduce the suspected difference with complete original generated libraries
   and the actual configured C++ controller, keeping successful compiler output
   distinct from a failed C++ mapping. Include qualified numeric and Boolean
   types and preserve existing reference refusals.
2. Add an explicitly versioned scalar-type observation to the full compiler
   capability. Derive getter identities from the locked model/naming helpers,
   and observe their compiler-normalized return types from the same library
   artifact as the descriptor and relation observers. Preserve older explicit
   capability formats and reject incomplete or malformed new documents.
3. Map canonical scalar types and optionality through the existing C++ carriers.
   Use per-generation consumption and model/source/toolchain binding. Replace
   the lexical scalar check only when this trusted type witness is present;
   retain explicit refusal for an actual missing representation or type mismatch.
   Do not infer storage from descriptor shape or silently fall back.
4. Verify real complete generation, C++ compilation and typed runtime reads for
   accepted aliases, and real compiler or mapping refusals for negative inputs.
   Run relevant Java/worker contracts, retained SDK reproduction and ordinary/
   optimized receipt checks. Preserve runtime ownership, layers, method names
   and existing compatibility decisions.
5. Record exact source/provenance transitions, remove owned disposable outputs,
   update the ledger from executed evidence and publish. Complete type/storage
   and configuration mapping and G13/G14/G17/P0-P7 remain the full objective.

This belongs to generation-time semantic resolution. It adds no dependency to
the C++ edge runtime and changes no Context allocation or lifetime domain.

## Initial reproduction and implementation

The [original diagnosis](parity/compiler_scalar_storage_diagnosis_5_0_6.json.gz)
freshly compiles complete original libraries for qualified `i64`, `f64` and
`bool`. All three original compiler processes succeed, while the configured
C++ generator refuses all three and publishes no SDK. This is an actual C++
mapping gap, not a reference compiler defect.

An explicit `teaql.rust_configuration.job.v3` capability now requests the
`teaql.rust_configuration.model.v2` original getter inventory. Its scalar
observer links the same original library, TeaQL core and JSON artifacts as
the descriptor and projection observers. The resulting
`teaql.rust_configuration.model_generation.v2` document adds a complete scalar
inventory and its observer/library fingerprints. Earlier explicit capability
and document versions retain their contracts.

The observer compares `TypeId` values within that one process against the
existing supported TeaQL value carriers and their optional forms. Names are
reported only after identity matches; unsupported types have an explicit
`unmapped:` marker. No numeric TypeId or process-specific ordering crosses
the protocol. Rust documents that
[`type_name`](https://doc.rust-lang.org/std/any/fn.type_name.html) is diagnostic
and does not guarantee unique names;
[`TypeId`](https://doc.rust-lang.org/std/any/struct.TypeId.html) supplies equality
for static types. The observer uses only their stable APIs and the pinned
compiler/sysroot, without invoking getters or constructing business entities.

The C++ producer maps this trusted identity to its existing carriers and checks
optionality and width before emission. The added configuration method does not
consume descriptor properties early, so the existing complete-consumption exit
still applies. Missing facts, foreign entities, library disagreement and wrong
or unsupported carriers reject. The lexical check remains only for preceding
explicit versions and unconfigured calls pending their full migration.

The first native consumer attempts retain a wrong source path and a missing
Rust trait import. A later optional-value probe reaches the expected original
`unwrap()` panic on a legitimate SQL NULL. Both observers now use the existing
`eval()` API, which preserves loaded NULL and still rejects NotLoaded. The first
complete observational run matches all ten actual getter/E/JSON/SQL results
across five SDK pairs and retains one original compiler refusal. Its initial
type-name-only observer remains historical; final identity qualification uses
the TypeId observer separately.

## Qualified execution

The [final TypeId execution](parity/compiler_scalar_storage_typeid_5_0_6.json.gz)
uses the actual upload controller, fresh independent Java classpaths and complete
original/C++ libraries. Its five successful models exercise these carriers:

| Model configuration | C++ getter storage | Actual reads |
| --- | --- | --- |
| Optional qualified double | `std::optional<double>` | Numeric value, SQL NULL, empty selection |
| Qualified signed integer | `std::int64_t` | Positive and negative values beyond 32 bits, empty selection |
| Qualified double | `double` | Positive and negative fractional values, empty selection |
| Qualified Boolean | `bool` | True, false, empty selection |
| Qualified single precision | `float` | Positive and negative fractional values, empty selection |

All **ten original/native results** match for actual SQLite query rows, typed
getters, E `eval()`, JSON output and generated SQL. C++ compilation asserts the
exact getter carrier. The observer obtains **35 scalar facts** across the five
models. Complete source inventories match the same original libraries consumed
by MIR, descriptor and scalar observation. A sixth model retains the original
compiler's `u64` storage refusal: compilation exits 101 and no C++ SDK is
published. Unsupported custom representations still reject; the gate does not
establish arbitrary custom-type support.

The [Java contracts](parity/compiler_scalar_storage_units_candidate_5_0_6.json.gz)
pass **262 tests**. The final TypeId worker passes **15 tests** under each Python
mode in its [separate receipt](parity/compiler_scalar_storage_typeid_worker_units_5_0_6.json.gz).
The [ordinary](parity/compiler_scalar_storage_controls_5_0_6.json.gz) and
[optimized](parity/compiler_scalar_storage_controls_optimized_5_0_6.json.gz)
execution readers each reject **14 corruptions**, including missing scalar
members, wrong carriers/artifacts, modified observers, input/output substitution,
hidden reference refusal and Boolean exit codes.

The [retained SDK reproduction](parity/compiler_scalar_storage_sdks_5_0_6.json.gz)
executes six generation/check actions and 308 subprocesses. All **80 manifests**
update exactly three existing producer fingerprints and add the scalar helper;
all **3,560 code/Assist files remain identical**. No generated file is patched
by hand. The [default SDK batch](parity/compiler_scalar_storage_default_sdks_5_0_6.json.gz)
also freshly regenerates **406 complete SDKs**, each identical to its retained
baseline. Its 406 earlier C++ compiler outcomes are explicitly reused: this
batch performs zero new C++ compilations. Both Python modes reject all eight
default-batch receipt corruptions.

The [source admission](../scripts/compiler_scalar_source_admission_5_0_6.py)
pins exact implementation and reader images, the complete SDK transition and
the actual execution receipts. Historical observations retain their original
worker/library binding. Registered image transitions admit the current source;
unknown fingerprints and source additions remain errors. Reader changes preserve
the original execution, domain-observation and corruption-control bodies.
The reader transition also covers the evaluator's driver self-identity guard;
its exact follow-up changes only that guard. Earlier execution records are not
rewritten or relabeled.

The [affected native checks](parity/compiler_scalar_storage_native_5_0_6.json.gz)
pass **4/4**: generated optional types, query return types, Checker integration
and the generator child/parent corruption guard. Their selected native targets
need no rebuild. All **1,819 preceding C++ source/fixture files** retain their
hashes; the new handwritten scalar observer is additional. Pinned C++ format
write/check, reference/ledger verification and module-path checks pass.

The successful final scalar execution removes **1,872,541,744 bytes** across its
parent and six compiler-job directories. The fresh default batch removes its
5,045,794 bytes. Scoped SDK subprocesses remove their own temporary outputs;
the parent removes its remaining 38,009 bytes. Exact retained receipt paths,
source images, failed attempts and compact logs remain in this repository.
No shared cache, unrelated build directory or other session is cleaned.

## Next dependencies

This closes the reproduced qualified-scalar spelling gap. It does not complete
the configuration grammar, model identity decisions or the P0-P7 goal.
G13/G14/G17 stay partial, with **14 qualified / 3 partial** capabilities.
Continue in this order:

1. Complete remaining compiler-resolved configuration/type/storage mappings and
   JSON input/E combinations against unchanged complete original libraries.
   Preserve explicit refusal when the reference cannot compile or a faithful
   C++ carrier is unavailable.
2. Resolve the separately documented Option-model and column-attribute boundary
   decisions. The three approved JSON, quoted-FK and invalid wire-profile
   exceptions retain their exact scopes; they do not decide these open cases.
3. Join the configuration/identity results to AP01-AP12 and the existing governed
   query, graph Save, Policy, audit, sink and Context ownership evidence for G17.
4. Complete the artifact/frontend and final native integration exits after the
   remaining semantic obligations are satisfied. Select module, sanitizer or
   MIPS checks when the corresponding source/lifetime/toolchain changes require
   them. Physical-router execution remains outside current verification.

The authoritative dependency list remains
[the current goal exit audit](current_goal_exit_audit_5_0_6.md). This checkpoint
adds no C++ runtime dependency, changes no production C++ ownership behavior and
does not claim a fresh full native, module, sanitizer or MIPS epoch.
