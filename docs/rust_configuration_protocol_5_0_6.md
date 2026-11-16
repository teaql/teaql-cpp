# Locked compiler projection protocol prototype

Stage 1 of the [compiler resolver plan](rust_configuration_compiler_resolver_plan.md)
has an executable native prototype. The generation-scoped producer resolver and
complete semantic routing remain subsequent stages. Capability status stays
**14 qualified / 3 partial**.

The [protocol implementation](../scripts/rust_configuration_protocol.py) reads
the unchanged generated domain's compiler MIR. It does not evaluate Rust
expressions. The [execution driver](../scripts/verify_rust_configuration_protocol_5_0_6.py)
freshly compiles the locked original generator, generates four complete School
domains, and invokes the locked Rust compiler. The original generator and runtime
revisions remain `45e1572c4956ca963410d8ea436ead3c8857087a` and
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Actual compiler and descriptor observations

The [completed execution](parity/rust_configuration_protocol_5_0_6.json.gz)
contains sixteen terminal subprocess outcomes:

| Configuration | Compiler result | Projection facts | Descriptor execution |
| --- | --- | --- | --- |
| Complete | Success | 19 | Six entities; later audit length 17 |
| Partial trailing type | Success | 18 | Six entities; earlier length 13 retained |
| Ignored unknown metadata tail | Success | 16 | Six entities; earlier length 13 retained |
| Used missing Rust type | Refusal, E0425 | No facts returned | Compiler diagnostic retained |

The 53 projection facts cover to-one and to-many relations, an unused
constant-function pointer coercion, a re-exported entity alias, fully qualified
paths and concrete `Self` types in Academy and ResearchAcademy. These are finite
fixture observations, not complete Rust grammar acceptance.

An omitted `json_type` retains the reverse relation descriptor without adding a
JSON projection call. Invalid trailing metadata and an unknown assignment stop
later metadata processing as the original macro specifies. The driver executes
the compiled descriptors to distinguish these cases from an absent relation.
Generated Rust source hashes remain unchanged across compilation and execution.

## Protocol boundary

The native [toolchain lock](../tests/parity/rust_configuration_protocol_5_0_6_toolchain.json)
pins Rust 1.97.1, Cargo, target configuration and 69 compiler/sysroot file hashes.
The prototype is qualified for `x86_64-unknown-linux-gnu`; other targets require
their own qualification. This does not establish MIPS layout acceptance.

Every JSON projection binds a generated source module/path, owner, relation,
cardinality, resolved Rust type and actual call/constant line numbers. Identity
operands must be unmodified constants in the same basic block. Independent
fixture membership detects missing or substituted projections. Eight ordinary
relation getter calls per successful domain have a separate, explicit inventory;
unknown consumers, missing getters and duplicate getters reject.

The MIR header explicitly warns that its human-readable format can change.
Unknown headers, call formats, source identities, operands or toolchains fail
explicitly. Compiler refusal yields no stale MIR facts. This is a bounded
prototype protocol, not a stable Rust compiler interface or an installed
production resolver. C++ mapping and generation assembly remain Stage 2.

The ordinary and optimized readers each accept the completed receipt and reject
23 corruptions. The controls cover type/cardinality/membership substitutions,
constant clobbering and block boundaries, unknown/duplicate/missing consumers,
source paths, descriptor provenance, compiler/target changes and stale refusal
results. All active checks remain unconditional under optimized Python.

The [affected native receipt](parity/rust_configuration_protocol_native_5_0_6.json.gz)
records two existing AP02 CTests passing in each Python mode. The
[source review](parity/rust_configuration_protocol_source_review_5_0_6.json.gz)
and its optimized peer bind 12,013 inputs. Only the two new Python sources,
their toolchain lock and an evidence-only G14 ledger extension change from the
published preimage. All 1,817 C++ input identities and 707 native executable
hashes remain unchanged; four source/metadata corruptions reject per mode.
These are selected acceptance checks and a bounded source review, rather than
a fresh full-suite execution.

## Reproduction and retained diagnostics

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_rust_configuration_protocol_5_0_6.py --execute --record /path/to/fresh_receipt.json.gz
python3 scripts/verify_rust_configuration_protocol_5_0_6.py --check --record /path/to/fresh_receipt.json.gz
python3 -O scripts/verify_rust_configuration_protocol_5_0_6.py --check --record /path/to/fresh_receipt.json.gz
```

The [initial receipt](parity/rust_configuration_protocol_initial_5_0_6.json.gz)
preserves a collector failure: the expected error code was E0412, while the
actual qualified missing-type diagnostic is E0425. Its complete flag remains
false. The [intermediate candidate](parity/rust_configuration_protocol_candidate_5_0_6.json.gz)
preserves the subsequent successful run before getter membership was tightened.
The final receipt recompiles all four domains with the strict protocol; it does
not relabel earlier executions as current evidence.

All three owned build directories are removed after their children exit,
reclaiming 1,192,703,397 bytes. Compact commands, inputs, MIR, descriptors and
diagnostics remain in eight compressed receipts totaling 12,541,604 bytes;
shared caches are retained. The next step is
explicit generation-scoped resolver integration, followed by C++ SDK and SQLite
runtime qualification.
