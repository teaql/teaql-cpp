# Constant identity SQLite execution

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This extends the [metadata/compiler audit](constant_id_contracts_5_0_6.md) to
actual public bootstrap execution for its seven source-compilable domains.

## Execution boundary

The original observer compiles the complete unchanged generated crate for each
model. It installs the original public module factory, SQLite provider, internal-ID
service and domain executor, then calls `UserContext::ensure_schema`. The C++
observer compiles the complete generated umbrella and invokes the corresponding
factories/runtime entry. Neither observer implements constant initialization,
querying, reconciliation, graph planning or persistence locally.

A fixture retains its SQLite connection while each initialization round creates
and actually destroys a fresh UserContext and ID service. This follows the
original assembly rather than silently retaining an allocator service across
rounds. The C++ default PMR resource is disabled. Observation values are explicitly
copied into the fixture scope before a User is destroyed.

Inputs cross seven models, six stored states (fresh schema, existing, scalar
drift, missing row, soft-deleted row, unrelated extra row), four original module
modes (plain, Checker, behavior, combined), and one/two/three rounds. The declared
matrix contains **504** scenarios and **1,008** observed rounds, plus warmups for
controlled existing states.

Every round compares complete physical Platform/Status rows and versions, the
ID-space table, before/after state, exact schema/bootstrap errors and active-root
result or error. Table absence is preserved separately from an empty table.
Fixture SQL only creates controlled drift/removal/deletion/extra data; actual
bootstrap repair is performed by the production runtime. There is no error/value
normalization or timing-field substitution.

## Zero identity boundary

The original parser and complete crate accept ID 0. At runtime the generic graph
planner treats it as unassigned and allocates a positive internal ID. Its next
bootstrap lookup still requests the modeled fixed ID 0, which is absent. Repeated
initialization therefore inserts another BASIC row.

For the fresh combined-module fixture, three rounds produce BASIC IDs 1, 3 and 4,
with EXTENDED remaining at 2. ID-space levels advance from 2 to 3 to 4. Each
operation succeeds and the active Platform root remains 1. This is original
behavior, not idempotent constant seeding; C++ preserves it explicitly. It is not
silently normalized, repaired or presented as a general bootstrap guarantee.
The default constant generation guard remains in force.

## Scope

The three source-invalid shapes (negative one/minimum and duplicate IDs) retain
their original compiler dispositions and C++ pre-emission rejection from the
previous audit. These runtime observers do not bypass those failures or alter
frozen artifacts. Saturated/maximal, hash-fallback, missing-ID and ordinary
positive models are tested with their authentic metadata.

This checkpoint does not close other scalar/temporal, root/dependency topology,
missing-identifier/seed metadata, custom capability or concurrent-conflict
obligations. All complete generator capabilities and phases remain unqualified.

## Reproduction

Prepare the original/C++ metadata fixtures using
[the preceding audit commands](constant_id_contracts_5_0_6.md#reproduction), then:

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_provider_sqlite
python3 scripts/verify_constant_id_execution.py --check
```

Each Rust binary is copied only after its compiler/linker has exited successfully;
the same rule applies to each native C++ binary. Complete generated crates use
separate test harnesses and frozen runtime patches. No generated artifact or
production algorithm is edited. This is a native source-comparison gate, without
a new production dependency or physical-device claim.

## Terminal verification

All **504/504** declared original/native scenarios agree and the stored corpus
reproduces against all seven live original and native binaries. Across 1,008
rounds there are 864 successes and 144 exact soft-deleted fixed-ID UNIQUE errors;
warmups succeed. Every compiler/linker exits successfully before its output is
copied or executed. Pinned format/check covers **1,019** handwritten C++ inputs.
See the [terminal record](parity/constant_id_execution_5_0_6.json) for actual outputs,
binary/driver/corpus hashes and limits. Production and generator bodies are
unchanged, so Java, sanitizer, module and MIPS gates were not repeated. Earlier
compiler/generation/profile evidence retains its independent scope.
