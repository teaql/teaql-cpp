# TypedEntityChecker construction and dispatch boundary

Constructor and erased-capability repair qualified through exact original/native
compiler controls, current native/imported execution and targeted ASan/UBSan.
Complete capabilities and P0-P7 retain their independent exits.

References: Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, complete
`teaql-runtime/src/checker.rs`. The reference is the frozen source, not the
original checkout's active branch.

## Reproduced difference

Rust declares `TypedEntityChecker<T,C>` without struct-level trait bounds.
Its `new` implementation requires only TeaqlEntity for T. The erased Checker
implementation separately requires Entity, Clone, Send/Sync and TypedChecker<T>.

The previous C++ class required Entity and matching TypedChecker inheritance
at class declaration. This rejected a metadata-only model, unrelated checker
payload and noncopyable entity before construction, even though the actual Rust
constructor succeeds. It also prevented naming an otherwise opaque generic type.

The [initial compiler witness](parity/checker_constructor_initial_gap_5_0_6.json.gz)
retains the complete original success and old C++ failure, including the previous
header. Existing value/dispatch tests passing did not establish this public
constructor contract.

## Runtime mapping

| Operation | Original Rust boundary | C++ mapping |
| --- | --- | --- |
| Name an opaque carrier | Generic type can be named without Entity/TeaqlEntity | Primary template has no model constraint; it stores no T object |
| Construct a carrier | T implements TeaqlEntity; C need not implement TypedChecker | Constructor requires TeaqlEntity and an explicitly managed payload origin |
| Obtain erased Checker behavior | Entity + Clone + matching TypedChecker; source Send/Sync | Partial specialization requires Entity, domain_copyable<T> and matching TypedChecker inheritance; native execution remains owner-thread confined |
| Register or invoke Checker | Only the Checker implementation is usable | Inert primary carriers have no Checker base or check_and_fix; existing registry/Context constraints reject them |
| Retain into another scope | Source owns C; native has the agreed explicit Context adaptation | Checked payload borrow and independent name copy; no implicit ownership extension |

`domain_copyable<T>` is the existing allocator-aware copy capability used by
`clone_in`. Ordinary C++ copy construction is insufficient for Context data.
The metadata carrier does not create an entity, supply missing typed callbacks,
or grant runtime registration authority.

The valid erased adapter keeps its existing materialization, loaded/dirty-field
handling, before/after comparison, required-field findings and sparse merge.
Both class forms use one shared pair of checked payload-borrow helpers. The new
primary carries the same scoped name and resource-borrow responsibilities;
there is no third lifetime, default allocator or shared owner.

## Verification obligations

The paired fixture constructs five original/native cases: metadata with plain
payload, metadata with typed logic, Entity with plain payload, Entity without
Clone with typed logic, and the valid Entity/Clone/typed combination. It checks
one descriptor call per constructor and the valid erased entity name. Opaque
nonmetadata type naming remains separate from constructor availability.

Four independent original/native negative compiler controls reject metadata,
wrong logic and missing Clone at erasure/registration, and missing TeaqlEntity at
construction. The existing generated wrong-adapter compiler test now attempts
registration after construction, preserving rejection at the actual source
boundary. Wrong generated logic, wrong entity and wrong callback tests remain.
No generated artifacts or producer code is edited.

Native ownership controls reject Global retention of a User-owned payload,
retain a Global-backed carrier through an explicit destination copy, delete the
source User, reject the expired source identity and confirm zero retained User
bytes. The managed Ref<C> constructor is exercised independently. Existing
valid-checker ownership, exception and reentrant-close bodies run unchanged.
Targeted sanitizer checks disable leak scanning; MIPS and whole-suite execution
are separate requirements.

## Current terminal evidence

The [constructor execution](parity/checker_constructor_execution_5_0_6.json.gz)
passes all five original construction cases, one positive and four negative
compiler programs in each language, and native5/imported5/sanitizer5 ownership/
dispatch selections. Original and native constructor output agrees exactly.
Successful builds precede every execution and final selected builds have no work.
The generated compatibility selection separately passes one positive and four
negative contracts. Existing native generated flows match 1,620 retained
outcomes and governed SQLite Save matches 540; these are current regressions
against their retained source corpora, not new producer/oracle reproduction.

The [current complete Checker replay](parity/checker_source_constructor_retry_execution_5_0_6.json.gz)
passes all five original source tests and 2,716 exact source/native/imported
observations, with eleven named checks per frontend. Its five constructor/
dispatch checks overlap the selection above and are not added again to an
invented unique-test total. The earlier
[failed fixture run](parity/checker_source_after_constructor_execution_5_0_6.json.gz)
is retained separately; it incorrectly caught logic_error for the correctly
rejected expired access, whose actual API exception is ReferenceError.

The [preservation proof](parity/checker_constructor_preservation_5_0_6.json.gz)
checks 3,457 files in 78 SDKs, 2,201 protected artifacts and eighteen original
checker/fixture/observer files. The entire valid check_and_fix body remains
byte-identical. Only checker.hpp changes production implementation. No generator,
generated artifact, dependency or ownership domain is added.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_checker_source_5_0_6.py --modules \
    --record /tmp/checker_source_after_constructor_fresh_5_0_6.json.gz
python3 scripts/verify_checker_constructor_5_0_6.py --sanitize \
    --record /tmp/checker_constructor_fresh_5_0_6.json.gz
```
