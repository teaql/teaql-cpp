# Field parameters and configured scalar storage

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: declared parser/producer/compiler and six-domain execution matrices
qualified; G05/P3 remains partial.

## Authentic model and type boundaries

Nine complete controlled domains distinguish field parameters, builtin fallback,
type inheritance and `_type_definition` overrides. The observer calls the real
parser, Rust naming helpers and complete domain generator. It records field/type
attributes, storage types and actual root/constant seed expressions. It does not
reimplement those rules or patch generated source.

| Configuration | Actual original storage | Original complete crate | Current C++ disposition |
| --- | --- | --- | --- |
| `unsigned()` | String fallback | Compiles | String; compiles |
| `integer(rustType=u64)` field parameter | i64; parameter does not alter the Type declaration | Compiles | int64; compiles |
| String alias with no override | String | Compiles | String; compiles |
| Integer-derived alias with no override | String fallback | Compiles | String; compiles |
| Integer type declared `rustType=i64` | i64 | Compiles | int64; compiles |
| String alias declared `rustType=String` | String | Compiles | String; compiles |
| Integer type declared `rustType=u64` | u64 | Nine compiler errors | Reject storage mismatch before emission |
| Integer-derived custom type declared `rustType=u64` | u64 | Seven compiler errors | Reject storage mismatch before emission |
| Integer type declared `rustType=String` | String | Seven compiler errors | Reject storage mismatch before emission |

The source does not provide an independent builtin unsigned recipe. Its standard
u64 storage remains IDs and foreign keys. The configured types above do not turn
the generator's existing seed/getter helpers into matching unsigned helpers.
For example, the builtin integer override still seeds `7_i64`; the custom unsigned
type seeds a quoted `"7"`. The original crates retain **17 E0308** and **6 E0277**
errors in total. Every exact primary diagnostic and complete original output
reproduces against the frozen producer/runtime.

The initial checkpoint established compiler and configuration behavior only.
The subsequent [execution checkpoint](type_configuration_execution_5_0_6.md)
qualifies actual bootstrap, decoding and typed Q/E for these six source-compilable
domains. Broader customized type/configuration acceptance remains separate.

## Prevent silent native storage drift

Before the repair, all nine C++ domains compiled. The three invalid source-type
configurations emitted different native storage: custom u64 became String, while
integer configured as u64 or String remained int64. Passing C++ compilation did
not validate agreement between configured metadata and model storage.

The active C++ producer now validates recognized scalar type declarations before
creating its output file list. It reads the Type-level source mapping, preserving
the source's treatment of a similarly named field parameter. When recognized
configured storage disagrees with the native carrier, generation fails with
`C++ configured scalar storage mismatch` and an entity/field coordinate. No
unsigned cast or String-to-number conversion conceals the mismatch.

All six accepted configurations retain matching observed native storage and
complete compilation. The validator preserves Business-ID and relation carrier
responsibilities. This is a focused standard-scalar guard; arbitrary opaque Rust
types and broader customized mappings remain unqualified. The original late
compiler errors and native early frontend rejection have distinct stages, which
the records preserve rather than claiming identical failure diagnostics.

The three initial native outputs are retained separately in
`cpp_before_storage_guard`, marked historical and unsupported. They were reproduced
in an isolated compiler directory from the exact pre-guard producer source hash;
the active producer and original checkout were not modified for that replay.
Current rejected outputs contain observations/provenance only, not a usable SDK.

## Reproduction and integration

All nine original outputs, six accepted C++ outputs and three rejected frontend
records reproduce by complete file set and bytes. The compiler audit verifies
unchanged original diagnostics against its initial record and measures actual
C++ member storage instead of assuming metadata agreement establishes it.
Central generator verification checks all retained artifacts and current producer
hashes; old native drift artifacts remain separate historical records.

```sh
source scripts/toolchains/activate.sh
python3 scripts/audit_type_configuration_contracts.py --check \
  --report docs/parity/type_configuration_5_0_6_compilers.json
python3 scripts/verify_generator_parity.py
```

The production change is confined to frontend validation. Existing emitted
standard examples and scalar candidates were regenerated with current producer
fingerprints. Their complete compiler gates remain intact, including the authentic
bool source failure. Temporal original errors and generated output bytes remain
unchanged; its producer-only provenance refresh retains the earlier record.
No runtime dependency, C++ ownership domain or generated execution algorithm was
introduced. Default constant/inheritance guards and all full capability/phase
requirements remain in force. See the
[terminal checkpoint](parity/type_configuration_5_0_6.json).
