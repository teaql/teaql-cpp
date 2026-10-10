# Consuming Entity and wrapper capabilities

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Entity construction, default into_json, audit_as/Audited access/consumption and
boxed relation extraction/injection do not require Clone for the entity or
relation payload. The original non-Clone fixture compiles and runs these actual
public methods in `entity_owned_api_oracle.rs`; no copied wrapper algorithm or
generator is used.

## Complete boxed delegation follow-up, 2026-10-08

The unchanged-source non-Clone fixture now calls the actual Box<T>
TeaqlBoxedRelations descriptor extension and error extraction paths in addition
to its earlier success/injection calls. Descriptor extension publishes the
payload property; extracting an empty row propagates EntityError's exact entity,
message and default source=None. The expanded original fixture builds and runs
successfully. The matching default-PMR-disabled C++ contract calls the production
BoxedRelations adapter, checks the descriptor allocator, and retains the complete
owned typed error in the caller's scope without adding Clone to relation payloads.

The source interface's three responsibilities remain separate. This adds no
provider/graph runtime implementation, dependency or generator change. The ten selected entity checks pass under ASan/UBSan with leak scanning disabled
(3.12 seconds) and MIPS/QEMU (6.56 seconds). Native full integration remains
pending. The later Timestamp public-field edit requires another consumer refresh,
now running independently in the native/sanitizer/MIPS directories. Complete
entity-source and stage acceptance remain open until current checks finish.

## Additional C++ restrictions repaired

The earlier `detail::move_to` instantiated its foreign-copy branch even when a
move remained inside one Context. Result/Audited/BoxedRelations advertised
allocator-aware const construction regardless of their payload capability;
the wrapper's constructor body failed only later. Audited and BoxedRelations
also lacked allocator-aware move constructors, making managed relocation select
const copying. A corrected native fixture with deleted copy and assignment
reproduced all three erroneous copy declarations and the consuming compilation
failures.

The shared movement helper now transfers same-owner state without compiling
an unavailable foreign copy. Result's const construction requires both payloads
to support an allocator-aware copy. Audited and BoxedRelations require their
payload capability and provide actual allocator-aware movement. Foreign
non-copyable retention throws before consuming the source. Result alternative
replacement still prepares before destruction and constructs in place, requiring
neither assignment nor Clone of its entity payload.

Container capability follows its actual elements: Optional copies use their
existing DomainCopy specialization, and PMR vectors follow element capability.
An Optional<Value> is copyable into another Context; Optional/Vector of a
non-copyable entity are not. C++ explicit Audited allocator-copy remains a
copyable-payload extension, not a claimed Rust Audited Clone implementation.

## Scoped evidence and pending gates

The unchanged source fixture runs the original Entity/Audited/boxed methods
with non-Clone entity and relation types. The repaired production-header native
contract passes with usable default PMR disabled. It checks owned audit
construction, same-owner registered movement, mutable access, original comment
application, default consuming into_json, managed Result movement and alternative
replacement, early foreign-copy rejection and boxed relation consumption.
An Optional<Value> expression explicitly copied into Global remains usable after
actual source User destruction, exercising nested allocator propagation and
truthful optional copying.

`entity_owned_api_contracts` is registered in CMake. The final full native consumer rebuild and selected ASan/UBSan/MIPS checks
completed successfully. These results include the repaired shared headers and
all named ownership contracts, rather than relying on the earlier 205-test
checkpoint.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin entity_owned_api_oracle --locked --offline
build/rust-original-roundtrip/debug/entity_owned_api_oracle
cmake --build build --target teaql_entity_owned_api_tests
ctest --test-dir build --output-on-failure -R '^entity_owned_api_contracts$'
```

These C++ moves retain the selected explicit Context lifetimes. Neither wrapper
nor Result introduces shared ownership. Custom non-owning captures/native
pointers retain their existing lifetime obligations. No production dependency or
generator change is added. Complete Entity/default/API/source-test and broader
stage acceptance remain open; physical-router execution remains unverified.

## Integrated acceptance on 2026-10-07

All final builds and tests completed successfully:

| Configuration | Result |
| --- | --- |
| Native Debug, all consumers and full CTest | 215/215 checks passed. |
| ASan/UBSan with leak detection | Fourteen selected checks passed 14/14. |
| OpenWrt MIPS and QEMU 74Kf | The same fourteen checks passed 14/14. |

The selection includes registration, BaseEntityData diagnostics, Entity Debug,
owned Entity/wrappers, default-method sequences, UTC conversion, existing Entity
expression, SafeExpression, SmartList and Context reference contracts. Named
native and MIPS probes match live original Rust: registration 42/42, parsing
1,481/1,481, Debug 289/289 inputs (5,202 outputs), defaults 93/93 and UTC 64/64.
Every selected target finished building before its tests ran. The source lock,
476-file pinned formatter, public declaration and 58-group inventory checks pass.
No production dependency or generator changes were introduced. Physical-router
execution and complete remaining stage/API/privacy/source-test acceptance are
still unproven; this checkpoint does not complete the six-stage objective.
