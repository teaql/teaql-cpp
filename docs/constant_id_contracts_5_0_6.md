# Constant identity metadata and compiler contracts

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This audit uses actual original metadata parsing and complete generated crates.
It does not replace parser behavior with an interpretation of numeric XML text.

## Original outcomes

Ten complete models cover explicit positive/zero/signed-maximum identities,
negative one/signed minimum, signed overflow/unsigned maximum, duplicate IDs,
nonnumeric fallback and a missing first ID. Original and C++ metadata observations
agree before the generation/compilation boundary.

| Input | Actual original metadata | Complete original Rust crate |
| --- | --- | --- |
| `-1` | -1 | Fails: negative identity appears in invalid binding names and unsigned expressions |
| `-9223372036854775808` | Signed minimum | Fails at the same generated binding/type boundary |
| `9223372036854775808` | `9223372036854775807` | Compiles |
| `18446744073709551615` | `9223372036854775807` | Compiles |
| `0` | 0 | Compiles |
| `9223372036854775807` | Signed maximum | Compiles |
| Duplicate `1`, `1` | Both identities are 1 | Fails with duplicate ID-candidate query methods |
| Nonnumeric first ID | Hash fallback 62970894 for Basic | Compiles |
| Missing first ID, explicit second ID 2 | First candidate receives 2, second receives hash fallback 1038262983 | Compiles |
| `1`, `22` | Both identities retained | Compiles |

The saturation and packed missing-ID behavior come from the existing producer
parser and metadata classes. C++ preserves them; the migration does not impose a
new interpretation. These are compiler/metadata observations, not proof that
zero, extreme or fallback identities are appropriate business model choices or
that their complete SQLite bootstrap behavior has been qualified.

The [initial audit](parity/constant_id_contracts_5_0_6_initial_audit.json) retains
all compiler errors. Seven original crates compile; three fail. Initially C++
already rejected duplicate ID candidate names, while negative IDs emitted malformed
bootstrap recipe names and failed in the C++ compiler.

## C++ rejection adaptation

`CppCoreGenerator` now rejects a negative parsed constant identity before returning
any generated files. The error names the entity and exact parsed ID. It does not
reject zero, positive signed maximum or source-saturated inputs. Existing duplicate
candidate-name rejection remains unchanged.

This is an explicit frontend adaptation of an original generated-code compilation
failure. It is not a claim that the original generator has the same exception or
that a failing original crate was repaired. The normal constant qualification
guard remains enabled.

All original generated artifacts, metadata observations and producer/model/file
hashes are retained in
[constant_id_contracts](../tests/generator/parity/constant_id_contracts/cases.json).
The test-only Java driver calls original parser/metadata/generation entry points;
C++ generation uses its actual qualification constructor. Rust compilation uses
complete unchanged generated crates with a separate harness and frozen runtime
path patches. C++ compilation includes the complete generated umbrella. Neither
side substitutes handwritten constant/filter logic.

## Reproduction

Build the original and C++ generator Maven reactors and derive their classpaths:

```sh
mvn -q -f .reference/teaql_code_gen/pom.xml -pl generator -am -DskipTests package
mvn -q -f .reference/teaql_code_gen/pom.xml -pl generator dependency:build-classpath \
  -Dmdep.outputFile=/tmp/teaql_constant_metadata_original_classpath.txt
mvn -q -f ../teaql_code_gen_cpp/pom.xml -pl generator -am -DskipTests package
mvn -q -f ../teaql_code_gen_cpp/pom.xml -pl generator dependency:build-classpath \
  -Dmdep.outputFile=/tmp/teaql_constant_metadata_cpp_classpath.txt
source scripts/toolchains/activate.sh
python3 scripts/audit_constant_id_contracts.py --check
```

The script also requires the existing locked Rust observer dependencies in the
Cargo cache. Cargo checks run offline against frozen source patches; no new
production dependency is introduced. Classpaths and compiler logs are temporary
development artifacts. Recorded outcomes keep exact source diagnostic codes,
messages and primary file/line anchors.

Broader no/multiple-root, missing identifier/candidates, seed/field metadata,
scalar/time, customization and actual execution obligations remain in the
[constant qualification matrix](constant_qualification_matrix_5_0_6.md).
Full G05/P3/P7 acceptance remains open.

## Terminal verification

All ten declared metadata/generation/compiler contracts pass: seven complete
original crates and C++ umbrellas compile; the three invalid source shapes retain
original failures and C++ rejects them before emitting files. Byte reproduction
checks every complete original artifact and file set. Focused Java checks pass
**38/38**; all **18** existing C++ workspaces/**216** files regenerate and reproduce.
Pinned format/check covers **1,018** handwritten C++ inputs. The
[terminal record](parity/constant_id_contracts_5_0_6_terminal.json) keeps source and
implementation hashes, Java reports, reproducibility output and initial C++
compiler diagnostics. Runtime bodies and lifetimes are unchanged, so sanitizer,
module and MIPS tests were not repeated. No default qualification guard was removed.

The later [SQLite execution checkpoint](constant_id_execution_5_0_6.md) qualifies
these seven source-compilable ID models over 504 scenarios. It explicitly retains
the original zero-ID non-idempotence rather than treating a compiler pass as
idempotent seeding evidence. The compiler audit above retains its original scope.
