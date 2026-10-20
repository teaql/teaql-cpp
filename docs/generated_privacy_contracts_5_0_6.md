# Compiled generated privacy metadata: Rust 5.0.6

Status: both reproduced metadata gaps are repaired. All 23 compiled source/native
configurations match, with current generator/native/reproduction gates passing.
G14/P1/P7 retain their remaining requirements.

The reference is producer `45e1572c4956ca963410d8ea436ead3c8857087a` and runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Unlike the earlier template-only
metadata check, this observer imports complete original Platform/Item/Child
entity files unchanged and executes their original TeaqlEntity derive macros.
It then calls the actual descriptor and SqlIntentRedactions::field_policy APIs.
It does not substitute descriptors built locally from template expectations.

## Original source responsibilities

| Responsibility | Contract and verification |
| --- | --- |
| Resolve declared masks | ObjectDescriptor resolves each declaration against that model's predefined fields, then inherits only already resolved parent fields. Unknown names do not become effective merely because a later child defines them. |
| Emit model mask keys | ObjectDescriptorNaming.canonicalAuditMaskFields supplies model keys. The entity template and derive preserve those strings unchanged; the derive does not rename a relation to its FK storage member. |
| Preserve declaration state | The generated entity attribute declares even an empty mask list. Metadata-only/default descriptors that never declare a complete policy retain their different runtime Unknown classification. |
| Interpret audit length | The derive recognizes integer and string literals and parses native usize. Other expressions and numeric overflow become None. Raw attribute text is not a C++ integer literal contract. |
| Carry identities | Entity/table/service identity, inherited scalar fields and explicit column names are observed from compiled descriptors. The fixture uses nondefault Item/Child tables, a renamed scalar column, a relationship FK and an optional scalar. |
| Apply field policy | Runtime lookup uses the actual property name; credential recognition checks property and column names before declared masks. The fixture verifies credential-column priority and Plain/Masked classification. |
| Execute wire/privacy consumers | Complete generated wire mappings, governed SQL/audit/log delivery and broader constant/candidate/dynamic/readonly/model configurations remain separate required G14/G12/G13/G16/G17 integration work. Descriptor emission alone does not accept those paths. |

## Two reproduced gaps

The [initial compiled comparison](parity/generated_privacy_contracts_initial_comparison_5_0_6.json.gz)
retains both successful original/native builds, complete generated file trees,
unchanged original entity bodies, descriptors and field-policy observations.

For a model relation named `platform`, the original descriptor contains mask
`platform` while its FK property is `platform_id`. The original field-policy
implementation compares the mask against the property name exactly, so this
generated FK is Plain. C++ previously replaced the mask with `platform_id` and
classified the FK as Masked. The earlier metadata-only comparison explicitly
mapped names to storage keys and consequently failed to detect the difference.
The C++ emitter now preserves original model mask keys. This records the original
generated relation-mask boundary; it does not claim that this source declaration
masks the FK or invent a new application masking API.

For `_audit_value_max_len="010"`, the original derive returns decimal 10.
C++ previously emitted `d.audit_max_length(010)`, which executes as octal 8.
The C++ adapter now parses supported source literals and emits canonical decimal
values. It retains ignored noninteger/overflow values and accepts verified radix,
underscore, suffix and quoted-decimal forms through the same metadata path.
Values wider than 32 bits use an unsigned literal and an explicit target-size
assertion; a 32-bit target must not silently truncate the declared limit.

The runtime descriptor, field-policy algorithm, privacy projection and Context
ownership implementations are unchanged. The repair belongs to generated
metadata rather than another copy of privacy rules in generated business code.

## Comparison scope and reproduction

The 23 configurations cover absent/default, leading-zero decimal, hex/octal/
binary, underscores, integer suffixes, quoted decimal/plus/invalid/overflow,
negative, Boolean, floating, path and parenthesized expressions, unsigned overflow,
the native u64 maximum, zero, whitespace and empty values. Each compiles three
original entity descriptors and the corresponding complete C++ facade.
The comparison preserves masks, declaration state, table/service, parsed limits
and property/column/field-policy tuples without output normalization.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_generated_privacy_contracts_5_0_6.py \
  --output /tmp/privacy_contracts_replay.json.gz --require-match
python3 scripts/verify_generated_privacy_metadata.py --check
```

Use a fresh output path: the auditor never overwrites retained evidence. Rust
registry versions and checksums must belong to the reference Cargo.lock. Temporary
C++ observers use the locked compiler, pinned formatter and actual runtime
archives. teaql_async is an interface target with no header-frontend archive.

The [first validation attempt](parity/generated_privacy_contracts_initial_5_0_6.json.gz)
retains its incorrect link request for a nonexistent teaql_async archive; the
subsequent initial comparison links successfully and proves the metadata gaps.
A concurrent Maven/classpath preparation attempt also interrupted generation;
its [complete failure](parity/generated_privacy_contracts_initial_validation_5_0_6.json.gz)
is retained and subsequent Maven/generation operations are serialized. Neither
failure is presented as successful verification or a runtime defect.

This checkpoint does not qualify arbitrary raw Rust expression/string syntax,
all model configurations, full generator assembly, protected-I/O execution or
complete G14. Those obligations remain in the ledger. Ordinary generated builds
have no new dependency; the adapter uses JDK BigInteger only during generation.

## Terminal evidence

The [repaired comparison](parity/generated_privacy_contracts_repaired_5_0_6.json.gz)
contains 23 successful original/native compilations, 69 complete descriptors and
322 property/column/field-policy tuples. Both initial original generated trees
and original derived results remain unchanged after the C++ repair. Generated
artifacts were regenerated through the actual producer, never patched by hand.

Java tests pass 51/51 and relevant native tests pass 10/10, including generated
model contracts, the runnable generated example, event models and original
privacy implementation controls. All 23 workspaces/276 files strictly reproduce.
The 44 SDKs were genuinely regenerated with every code byte unchanged before
updating producer hashes; strict replay then passes. The first strict check's
[stale-provenance failure](parity/generated_privacy_contracts_sdk_initial_5_0_6.json.gz)
is retained. See [SDK reproduction](parity/privacy_contracts_sdk_regeneration_5_0_6.json).
The earlier 12-model/48-record metadata corpus also replays unchanged with the
correct model-key interpretation. Formatting checks 1405 handwritten C++ files.

A focused [MIPS32 literal guard](parity/generated_privacy_limit_width_5_0_6.json)
accepts a small value and rejects the u64 maximum with the emitted target-size
assertion. Its initial basename-sensitive compiler-wrapper invocation failure
is retained. This is a syntax/width check, not a complete MIPS build, QEMU run or
physical-device test. No sanitizer profile was rerun because no lifetime or
allocation mechanism changed. Named-module APIs and existing SDK code bytes
are unchanged; this checkpoint does not claim a new module release gate.

The [terminal record](parity/generated_privacy_contracts_5_0_6.json) distinguishes
metadata/compiler execution from protected SQL/audit/log integration. G14 remains
partial for complete model/configuration, generated wire mapping and protected
consumer qualification; no whole P1/P7 acceptance is implied.
