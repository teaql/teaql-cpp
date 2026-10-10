# Core Business ID responsibility-group review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/business_id.rs`. This reviews the complete production file,
not runtime generation strategies, permutation, database allocation or save
orchestration. Those responsibilities remain in their own modules.

## Complete source surface

| Requirement | Concrete C++ contract and reviewed evidence |
| --- | --- |
| Four constants | Default and legacy profile names, six default digits and the exact u64 domain size are public constants with identical values. |
| BusinessIdErrorCode | All seven variants, Copy/Clone/PartialEq/Eq and exact derived Debug names. The original-call matrix includes all 49 pair comparisons. No source Default/Hash/serde requirement is invented. |
| BusinessIdError | Code/message, owned constructor, explicit scoped Clone, structural equality, exact Debug and `Code: message` Display. The default Error cause chain is empty. The missing native source() API was reproduced as a compiler error and repaired to return nullptr without allocation. |
| BusinessIdDefinition | All seven fields, daily_sequence/legacy_daily_sequence and max_sequence. Permuted defaults require exactly six digits; other profiles accept digits 1–19 with the exact checked-decimal bound. Unknown profiles are not rejected by this core method. Default factory values and exact rejection text match the original. |
| BusinessIdScope | All four fields and canonical_key. Percent escaping precedes pipe escaping; field order and separators are preserved, including empty/Unicode/embedded-control content. |
| BusinessIdGenerationRequest | Definition, domain root, aggregate type and business date. Structural Clone/PartialEq/Eq/Debug cover every definition field and all three other fields. Explicit-owner construction takes a definition/string snapshot; this documented C++ ownership adaptation avoids retaining a foreign User borrow. |
| BusinessIdPlan | Scope, prefix, optional date, digits, separator, initial and maximum sequence. validate_allocation_range only requires initial <= maximum. Equality distinguishes None from Some(empty); no profile/digit validation is invented at this entry. |
| BusinessIdAllocation / BusinessIdValue | Complete scope/sequence pair and one String tuple value, with scoped Clone, structural equality and exact derived Debug. The tuple field is spelled value in C++. |
| BusinessIdAllocator | Separate const virtual allocate with the concrete Plan and Result<Allocation,Error>; caller selects the result allocation domain. No default implementation is supplied. |
| BusinessIdProfile | Separate const virtual plan/format/validate, complete request/plan/allocation/definition inputs and concrete typed Results. No default methods are supplied. Shared-service implementations retain the source Send+Sync responsibility; the memory kernel remains owner-thread confined. |
| BusinessIdSlot | current_business_id returns an optional borrowed view, is_new_aggregate reports state and assign_business_id consumes the owned value. Native positive/negative concept checks and actual dispatch preserve all three responsibilities. |
| Reexports and protocols | Core public facades expose these models/interfaces and Debug writers. None of these source types derives serde; fixture JSON is observation data, not a production wire contract. Runtime/service/provider behavior is not accepted by this core review. |

Field changes are observed independently, including every nested definition or
scope field. Each pair also checks symmetry and equality with an actual scoped
clone; a self-clone assertion alone cannot prove that a field participates.
The complete expanded matrix has 1,283 actual Rust inputs: 1,188 earlier
constructor/range/key/error cases plus 95 equality observations. The error
observations additionally call the original Error::source default directly.

## Context ownership

business_id_tests.cpp disables default PMR and calls all three native interface
responsibilities through their separate abstractions. Outputs select either
User or Global explicitly. Definition/request/scope/plan/optional text/allocation/
value/error state is retained by explicit destination construction; nested
allocators are verified. Failed foreign assignment preserves both source and
destination. Large canonical keys grow in arena storage and every abandoned
copy block returns to the destination upstream.

core_aux_debug_tests.cpp complements this with 8KiB values, all seven owned
Business ID models, deep Global relocation, actual source User deletion,
rebound access through a new User and complete unchanged diagnostic output.
The 94-input actual-source auxiliary corpus observes the seven models, all error
codes and nine inherited formatter modes alongside other auxiliary models.
Those other models are not accepted by this Business ID review.

Request snapshots are an explicit ownership adaptation. The source request
borrows its immutable definition and strings; C++ callers retain an owned
snapshot in the chosen scope. No third ownership domain or shared lifetime
extension is introduced. Resolved native references/views remain temporary.

## Verification checkpoint, 2026-10-07

The repaired native probe matches all 1,283 live-original inputs. The source()
compiler reproduction now succeeds. The four native Business ID/auxiliary
contracts and differential checks pass 4/4 in 19.79 seconds. Pinned-source and
565-file clang-format write/check pass.

The rebuilt ASan/UBSan selection with leak detection passes 4/4 in 26.34 seconds.
The rebuilt MIPS/QEMU selection passes 4/4 in 18.22 seconds; an explicit live
comparison also matches all 1,283 source calls. The initial MIPS differential
registration omitted CMAKE_CROSSCOMPILING_EMULATOR and reached a default binfmt
CPU, which raised SIGILL. Running the same binary with the configured QEMU 74Kf
passed. The CMake registration now forwards the configured emulator; its fresh
CTest rerun passes without changing or weakening observations.

A complete native consumer rebuild and final probe/CMake refresh now pass
281/281 in 73.07 seconds; Ninja reports no pending work. The core Business ID
responsibility group is accepted on the complete source table and scoped
ownership evidence above. The preceding complete native 281/281 in
67.65 seconds accepted runtime error/completion changes before this Business ID
header repair. It is not substituted for this final integration. Runtime and
complete stage acceptance remain open.

```sh
cmake --build build --parallel 4 --target teaql_business_id_tests \
  teaql_business_id_probe teaql_core_aux_debug_tests teaql_core_aux_debug_probe
ctest --test-dir build --output-on-failure --parallel 2 \
  -R '^(business_id_contracts|business_id_rust_parity|core_aux_debug_contracts|core_aux_debug_rust_parity)$'
python3 tests/parity/compare_business_id.py --cpp build/teaql_business_id_probe \
  --rust build/rust-parity/debug/business_id_oracle
python3 tests/parity/generate_business_id.py \
  --rust build/rust-parity/debug/business_id_oracle
```

For MIPS use the cross build and `--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'`.
Use `ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1` for sanitizer CTest.
Neither emulation nor this core contract establishes physical-router execution
or complete stage acceptance.
