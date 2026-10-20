# Opaque configured scalar storage

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Status: three original producer/compiler configurations and C++ frontend
rejections are recorded. This addresses a concrete P3 scalar-storage gap;
G05/P3 remains partial.

The original type declaration can name an arbitrary Rust carrier. C++ previously
ignored an unrecognized explicit carrier and emitted String storage according
to the field recipe. That could produce a usable SDK with a different declared
domain type. The C++ frontend now rejects unmapped explicit carriers before
creating its output file list. Recognized storage mappings still retain their
existing equality/mismatch checks; a field-level rustType parameter continues
to follow the source's different handling.

Three complete constant/Platform domains declare OpaqueToken, a qualified u128
and Vec<u8>. The frozen producer generates complete Rust artifacts, but their
actual compilation fails with 4, 11 and 11 errors respectively. Original files,
model/producer/runtime hashes and raw compiler diagnostics are retained. Current
C++ generation rejects all three with an entity/field coordinate and produces
no partial SDK. Original late compilation failure and native early rejection
are distinct failure stages; no runtime behavior is claimed for a failed crate.

The generator changes neither standard runtime Value carriers nor allocation
domains. No dependency, fabricated opaque adapter or implicit text conversion
is introduced. Selected Java regression passes 41/41. Twenty retained workspaces
regenerate; 41 unrelated complete C++ contract SDKs regenerate with identical
bytes. Original/reference checkouts remain unchanged.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_opaque_type_configuration.py
python3 scripts/verify_generator_parity.py
```

The audit preserves already retained original bytes and compiles complete crates
in temporary projects. See the [compiler/frontend record](parity/opaque_type_configuration_5_0_6.json)
and [fixture directory](../tests/generator/parity/opaque_type_configuration_contracts/).
