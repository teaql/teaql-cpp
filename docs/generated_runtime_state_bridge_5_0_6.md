# Generated borrowed runtime state: Rust 5.0.6

Status: adapter emitted, all 78 SDKs regenerated and strictly reproduced, and
runtime/ownership selections pass native **9/9**, imported modules **6/6** and
targeted ASan/UBSan **4/4**. Each profile matches the six complete actual generated
source observations. The terminal compiler gate passes **55 positive / 141
negative** contracts. The generated borrowed-root adapter is qualified in this
scope. Complete G13/G17 and P0-P7 remain separate.

## Combined source responsibility

The frozen generator's entity struct template applies `teaql_entity` and derives
`TeaqlEntity`. The locked runtime's `teaql-macros/src/derive_impl.rs` implements
`__teaql_runtime_state_any` by borrowing the generated entity's existing state
member. Looking only for a method body in the generator template misses this
responsibility. The C++ core default correctly returns no state for ordinary
entities; generated models need a distinct nonempty implementation.

The [runtime-first checkpoint](runtime_state_bridge_5_0_6.md) qualified shared
ordinary dynamic-save dispatch, explicit absent/wrong carriers, legacy typed
compatibility and Context ownership before this generation change. Its terminal
receipt remains historical evidence for unchanged shared runtime mechanics.

## Reproduced missing generated behavior

The [initial witness](parity/generated_runtime_state_bridge_initial_5_0_6.json.gz)
retains six complete actual original/C++ results: ordinary Record and inherited
ChildRecord, each with empty, one-byte and 2,048-byte payloads. The original
observer uses the unchanged generated Rust crate and actual locked derive and
runtime. All six original entities expose their existing state. All six C++
entities inherited the empty core hook. The initial C++ ownership assertion also
fails before exercising copies or destruction.

The source observer has an isolated manifest and lock. Every registry version
and checksum belongs to the exact runtime reference lock; it does not inherit
the older field-layout observer's dependency drift. The corpus binds all original
generated files, their provenance, the derive source and the observer inputs.
An initial fixture-only Vec-to-Arc argument error is retained in
[its archive](parity/generated_runtime_state_bridge_initial_fixture_5_0_6.json.gz).
No original generated source or runtime implementation was patched to compile it.

## Thin generated adapter

The independent cpp_support workspace adds these methods to shared Model:

```cpp
std::optional<AnyRef> runtime_state_any() const& {
    return std::optional<AnyRef>(std::in_place, root);
}

std::optional<AnyRef> runtime_state_any() const&& = delete;
```

AnyRef borrows the existing member. The adapter adds no root owner, member,
allocation, Context parameter, lookup algorithm or lifetime extension. Ordinary
and flattened inherited models obtain the same protocol. The existing typed
`entity_runtime_state` getter and generated typed-ledger Save path are preserved.
This is a kernel protocol, so its snake_case spelling follows interface
responsibility rather than the location of generated code.

Const lvalues may borrow; temporaries and explicitly moved expressions may not
call this hook. These constraints prevent the simplest temporary-object escape;
an ordinary raw borrow still becomes invalid when its object or Context closes.
The hook is not a general checked-reference wrapper.

## Artifact and naming preservation

The [before archive](parity/generated_runtime_state_bridge_before_5_0_6.json.gz)
retains the complete prior template/test body and SDK manifest identities.
The [regeneration receipt](parity/generated_runtime_state_bridge_regeneration_5_0_6.json.gz)
covers all **78** active SDKs. Only **64** support headers acquire the exact
adapter; invalid-model outputs remain their original outputs. Removing that
exact method pair and applying the pinned formatter reproduces each entire
previous support header byte for byte. Every other generated file remains
identical; manifests update their actual producer/file hashes. All **2,201**
protected original/historical artifacts remain unchanged.
The subsequent [strict reproduction](parity/generated_runtime_state_bridge_reproduction_5_0_6.json.gz)
regenerates every SDK again with no permitted code or provenance differences.

The generator regression includes order_status, category, person and inventory
plus an inherited inventory model. It preserves centralized pluralization and
asserts both the thin lvalue hook and deleted temporary entry. The focused Java
selection passes **81** tests, recorded in
[the receipt](parity/generated_runtime_state_bridge_java_5_0_6.json.gz).

## Qualification commands and boundaries

Use the locked tools and fresh receipt paths. Strict reproduction accepts no
code or provenance differences:

```bash
source scripts/toolchains/activate.sh
python3 scripts/regenerate_runtime_state_adapters_5_0_6.py \
    --record /tmp/generated_state_reproduction.json.gz
python3 scripts/verify_generated_runtime_state_bridge_5_0_6.py \
    --profile native --record /tmp/generated_state_native.json.gz
```

The same verifier has modules_native and targeted sanitize profiles. Their
terminal receipts are [native](parity/generated_runtime_state_bridge_native_5_0_6.json.gz),
[modules](parity/generated_runtime_state_bridge_module_5_0_6.json.gz) and
[sanitizers](parity/generated_runtime_state_bridge_sanitize_5_0_6.json.gz).
Each selected build completes before execution and finishes with a no-work
build. Sanitizers disable leak scanning as required by project policy.
Complete
source observations retain stable borrow identity, wrong-type rejection, initial
and final comments, typed/core shared state and generated mutation/readback.
Native ownership contracts separately verify the actual root address, source
allocator, shared ledger, checked expired token, actual User destruction and
explicit long-string Global retention with default PMR disabled. They never
dereference an expired raw AnyRef or state.

The full generated API compiler verifier includes one positive lvalue contract
and two negative temporary/moved-expression contracts. The terminal run uses the
locked compiler, four workers and operation-local PCH storage in `/dev/shm`.
All **196** contracts pass, including those three new constraints. The
[compiler receipt](parity/generated_runtime_state_bridge_compiler_5_0_6.json.gz)
retains its zero exit code, complete verifier output and exact verifier hash.
The earlier
[runtime checkpoint](parity/generated_runtime_state_bridge_runtime_checkpoint_5_0_6.json.gz)
keeps its historical compiler-pending scope. The
[final acceptance receipt](parity/generated_runtime_state_bridge_acceptance_5_0_6.json.gz)
binds the terminal compiler result to the current emitter, SDKs and source/runtime
gates; the earlier record is not rewritten as complete adapter acceptance.

Current structural checks report 736 registered native tests, 875 canonical
imported bodies and 1,506 catalog exports. They are provenance and registration
checks, not a full native run or complete semantic acceptance.

Existing graph Save, generated API, dynamic-field and inheritance contracts are
integration checks in their stated selections. They do not turn six borrowed-root
observations into complete graph persistence or whole-file acceptance. At that runtime checkpoint,
source dispositions were **21 accepted / 103 pending**. The later
[standing external-scope review](external_telemetry_scope_5_0_6.md) records the
current **21 accepted / 101 pending / 2 excluded_external**, without adding an
implementation acceptance. The subsequent complete helper and presentation-data
reviews advance the current source state to **24 accepted / 98 pending /
2 excluded_external**. This adapter acceptance adds no whole-source disposition.
It does not resolve other model configurations, quoted-FK/naming dispositions, full current
native integration, G13/G17 or P0-P7. No MIPS or physical-device result is added.
