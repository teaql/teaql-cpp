# Constant metadata and frontend failure contracts

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the 13 generation/compiler configurations reproduce and all eight
source-compilable domains pass their declared SQLite execution matrix.
G05/P3 and complete generator acceptance remain partial.

## Preserve actual parser and generator behavior

Each fixture passes through the authentic KSML parser and complete frozen Rust
generator. The counterpart uses the independent C++ producer's qualification
entry. Both emit their whole domain or a recorded failure; no generated source
is repaired by hand. Complete original/native file sets and model, observer,
producer and runtime hashes are retained alongside every fixture.

`ObjectDescriptor.isRoot()` describes a structural root. Domain Root selection
additionally excludes constant descriptors. A standalone constant therefore
cannot satisfy the ordinary root requirement, even when its structural root
flag is true. Two ordinary roots also fail regardless of declaration order.

| Input configuration | Original outcome | Native outcome |
| --- | --- | --- |
| No ordinary root | StringTemplate suppresses the bootstrap naming exception and emits an incomplete crate; one E0425 compiler error | Reject before emitting an SDK: exactly one Domain Root required |
| Two ordinary roots, both declaration orders | Same incomplete-runtime failure, one E0425 per crate | Same early root rejection |
| Missing identifier declaration | Compiles; no identity constants | Compiles; same parsed metadata |
| Identifier names an absent field | Compiles; no identity constants | Compiles; same parsed metadata |
| No explicit value rows | Compiles; declaration text supplies a candidate | Compiles; same parsed metadata |
| Empty identifier value | Compiles; no identity constants | Compiles; same parsed metadata |
| Missing identifier value in a row | Compiles; declaration text supplies its missing value | Compiles; same parsed metadata |
| Missing non-identifier seed field | Compiles; source fallback field data remains | Compiles; same parsed metadata |
| Comma inside identifier value | Compiles; comma splitting produces two identity constants | Compiles; same identities and fallback ID |
| Candidate names collide after naming conversion | Complete crate fails with two E0592 duplicate predicate definitions | Reject before emission with the existing candidate collision contract |
| Unicode identifier value | Complete crate compiles | Complete umbrella compiles |
| Malformed XML | Parser rejects the input | Same parser exception type and message |

There are **eight** source/native compile successes, **four** original compiler
failure configurations containing **five** compiler errors, and **one** identical
parser failure. The rejected native outputs contain only the observation record;
they do not expose partial generated headers as a usable SDK.

These frontend failures occur at different stages. Early C++ rejection is an
explicit adaptation of an unusable original generated library, not a claim that
the two diagnostic messages or compiler processes are identical.

## Declaration text and splitting are observable contracts

Do not replace underspecified inputs with stricter guessed validation. The
`no_candidates` fixture has no explicit `_value` rows, but the real parser
derives `STATUS_STRING()` with raw candidate value `String()` and ID
**1,717,331,758** from the `string()` declaration. It is not an empty parsed
candidate collection. The missing identifier value fixture derives the same
candidate name with its supplied ID **1**.

For the comma fixture, a single row with code `A,B` becomes two identity
constants: `A` at ID **1**, and `B` at fallback ID **66**. Missing identifier
declarations, unknown identifier names and explicitly empty identifier values
remain distinct inputs whose identity-constant collections are absent.

Missing non-identifier values also expose the parser's independent candidate
columns. In the fixture where the first row omits name and the second names
`Second`, original initialization gives ID 1 the name `Second` and ID 22 the
fallback name `1`. It does not preserve an empty first-row cell. The execution
observer retains this actual outcome instead of repairing row alignment locally.

The metadata observations record full candidate ID/key/value/line information.
Whole-crate compilation is required: inspecting one request snippet would miss
the suppressed-runtime and duplicate-definition failures.

## Supported-domain execution and lifetimes

The eight source-compilable domains have dedicated native and module observers.
They use real generated module factories, SQLite schema/bootstrap, internal-ID
services and typed Q/E reads. Six database states, four factories, one/two rounds
and diagnostic logging enabled/disabled form 96 scenarios per domain.

Every round has a fresh UserContext. Native retains the connection and explicitly
copied observations in GlobalContext, deletes the actual UserContext, then reads
the database again. The default PMR resource is disabled. Physical rows, versions,
ID floors, active root, complete errors, scalar/E values and SQL/binding/intent
diagnostics are compared without rewriting IDs or source failures. Timing fields
are outside the observer output.

All **768** scenarios agree exactly across **1,152** Context rounds,
**1,080** returned typed scalar/E rows and **3,052** SQL diagnostic entries.
All **1,152** typed queries succeed. Bootstrap succeeds in **1,032** rounds;
the other **120** retain the original soft-deleted fixed-ID UNIQUE failure.
Database snapshots before and after actual User deletion agree in every round.

Native and modules each pass **8/8** CTest entries. Live original execution
reproduces all eight retained corpora; all 13 complete generated file sets and
compiler dispositions reproduce. **396** canonical consumer/fixture bodies
verify, and pinned format/check covers **1,287** C++ files. The
[terminal record](parity/constant_metadata_contracts_5_0_6_terminal.json) retains
these results, hashes and command outputs.

## Reproduce

```sh
source scripts/toolchains/activate.sh
python3 scripts/audit_constant_metadata_contracts.py --check \
  --report docs/parity/constant_metadata_contracts_5_0_6.json
python3 scripts/verify_constant_metadata_execution.py --check
ctest --preset native --output-on-failure -R '^constant_metadata_.*_rust_parity$'
ctest --preset modules_native --output-on-failure -R '^module_constant_metadata_.*_rust_parity$'
```

Build the corresponding `teaql_constant_metadata_<case>_probe` and
`teaql_module_constant_metadata_<case>_probe` targets first. Generation and Rust
comparison use development tooling; normal C++ execution still needs only the
existing runtime and SQLite.

The initial audit is preserved separately. Native fixture regeneration uses the
repository's pinned formatter configuration. The controlled metadata-regeneration
flag defers only native artifacts; original provenance remains checked, and the
flag cannot be combined with completion qualification.

Default constant generation remains guarded. Dependency cycles, broader seed and
naming configurations, resource replacement, clock overrides and all remaining
capability/phase integration requirements are separate. No sanitizer, MIPS or
physical-device result is implied by this checkpoint.
