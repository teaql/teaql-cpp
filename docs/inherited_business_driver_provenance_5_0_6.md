# Adapted inherited-business source provenance, Rust 5.0.6

The existing inherited Business ID observers now have isolated, reproducible
bindings for **60 governed-save inputs** and **20 update/retry inputs**. All
**80 adapted-source/corpus/native observations** agree. A fresh original-source
check still reports the expected **20 compiler errors**. This is explicitly
adapted execution; untouched producer and original-artifact acceptance remain
false.

The runtime reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the
generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Original failure and reference dependencies

The old `rust_inherited_business_compile/Cargo.lock` contains **46 registry
identities** outside the current reference lock. Its files and historical
failure record are preserved. They are not relabeled as a reference-locked
execution.

The new `rust_inherited_business_original_source_5_0_6` project points to the
unchanged original compile driver and generated SDK. Its separate lock uses
only reference registry versions and checksums. In the owned replay,
`cargo check --offline --locked --message-format=json` must exit **101** with
exactly the original 20 error codes, messages and source locations. The missing
`DocumentNumber` and `DocumentDocumentNumberBusinessIdSlot` declarations remain
the recorded `E0425`/`E0433` failure. A successful compilation would fail this
guard rather than silently change the source disposition.

## Adapter and executed behavior

The replay copies the full **610-file runtime archive** and **75 project inputs**
into an owned repository directory. It runs the unchanged
`build_inherited_business_binding_bridge_5_0_6.py` script there. The root runtime
lock and a task-owned sibling copy of the producer lock are explicit inputs;
the script does not access a shared derived build directory.

The adapter recreates **43 library files**. Exactly five entity files differ:
the parent identifier tuple field gains crate visibility, and four descendants
gain the shared-type import and the original parent slot body rebound to their
carrier type. Slot method bodies, Checkers, runtime and query implementations
are preserved. The resulting hashes, prefixes, appended byte counts and parent
slot hash must equal the historical explicit adaptation record. Original SDK
files remain unchanged throughout both compiler phases.

The unchanged adapted harness and its lock are reused. Cargo metadata must bind
all six TeaQL packages to the owned archive and the generated package to the
owned derived library. All registry identities/checksums must come from the
reference lock. Cargo builds all four original harness binaries with Debug
assertions and a private target directory. The five-model helper executes;
the lookup helper is compiled without a new lookup-semantic claim.

Original input recipes are reconstructed without reading expected outputs.
The source and native observers use the same fixed business clock and public
fixture key material. They exercise inherited field checks/fixes, Business ID
allocation, audited persistence, write failures, updates and retries. Full raw
output is retained and compared with the unchanged corpora, preserving JSON
types and cardinality. No output normalization is added.

## Verification

- Original producer guard: **20/20 expected compiler diagnostics**, using the
  reference dependency identities.
- Derived source: **43/43 file hashes** and five-model helper output agree.
- Adapted source/corpus/native: **80/80 observations** agree.
- Negative evidence controls: **35/35 rejected**, including concealed original
  failure, missing adapter lock, changed derived bodies, false untouched-source
  claims and substituted key material.
- Affected native tests: **5/5 passed**, including the inherited lifetime unit
  test, both comparators and both provenance checks.
- Previous private key/classifier checks: **2/2 passed**, including 31 controls.

All **305 direct-corpus locators / 323,568 primary observations** now have
disjoint source-binding records under each observer's declared private-function,
adaptation and profile limits. This does not qualify the original broken SDK,
all custom/protected callers or complete generator parity. Registry
**934/305/197** means total tests/direct entries/custom dispatchers; it is not
a full 934-test execution. Capability states remain **14 qualified / 3 partial**.

All **1,814 C++ inputs**, producer implementations and **80 generated SDKs /
3,560 files** are preserved. Complete metadata/configuration review, custom
and protected-governance paths, current full integration and the separate
column-attribute decision retain their requirements. No sanitizer, MIPS/QEMU
or physical-device execution is added here.

## Evidence, cleanup and reproduction

The registry is `tests/parity/inherited_business_driver_contracts_5_0_6.json`.
Compact records in `docs/parity/inherited_business_driver_*_5_0_6.json.gz`
retain the before snapshot, preparation, original failure, adapter and complete
execution, bindings, controls, affected gates, inventory, partition, guards and
milestone. The first replay stopped because its isolated input catalog omitted
`upstream.lock.json`; its failure and preparation receipts are retained with
the `missing_root_lock` qualifier. The catalog now checks this dependency before
building. That failed replay's owned directory was removed after its child
exited; its temporary byte count was not measured. Successful replay and gate
cleanup sizes are recorded separately in the milestone.

```bash
source scripts/toolchains/activate.sh
cmake --preset native
python3 scripts/inherited_business_driver_provenance_5_0_6.py verify \
  --contracts tests/parity/inherited_business_driver_contracts_5_0_6.json \
  --execution docs/parity/inherited_business_driver_execution_5_0_6.json.gz
ctest --preset native --output-on-failure \
  -R '^(generated_inherited_business_lifetime_contracts|inherited_business_(governed|update)_rust_parity|parity_inherited_business_driver_(contracts|controls))$'
```

Use `replay` with a fresh `--record` path to repeat both Rust compiler phases.
Preserve historical receipts rather than overwriting them.
