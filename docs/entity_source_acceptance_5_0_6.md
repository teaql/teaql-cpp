# Complete core Entity source review: Rust 5.0.6

Status: complete core source accepted under EN01-EN12 and the explicit
ownership/language adaptations below. Native **16/16**, modules **16/16** and
targeted ASan/UBSan **8/8** pass. Source dispositions advance to
**21 accepted / 103 pending**. This review covers the complete 538-line
`teaql-core/src/entity.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Runtime callers, generated overrides
and complete G13/P7 remain separate responsibilities.

## Actual original defects

The [original/native witness](parity/entity_source_initial_5_0_6.json.gz) retains
all 230 inputs/results and the exact pre-repair production header. It reproduces
**111 differences**:

- One metadata call cannot obtain the default unindexed layout protocol in C++.
- Twenty-six empty/Unicode-blank audit reasons produce the old generic error
  instead of the source MutationIntent code, field and repair guidance.
- Eighty-four successful boxed relation calls lack JSON presentation dispatch.
  Ordinary mutation injection is not a substitute for an overridden JSON bridge.

The original observer invokes the complete frozen core traits, including Box
implementations and the real Audited constructor. It does not copy a validator,
relationship dispatcher or presentation implementation. The Rust panic hook is
silenced only in the test process so caught failures can be recorded as JSON.
Rust constructor panic and C++ invalid_argument are the explicit language failure
boundary adaptation; success/failure, complete error text and values compare
without normalization. LF framing preserves U+0085/U+2028/U+2029 inside valid JSON.

The failed native lifetime check and all original/native outputs remain in the
witness. Initial fixture compile mistakes and an unregistered stack-definition
borrow are retained separately; they are fixture failures before qualification.
The definition is now registered through UserContext before building its carrier.

## Shared core mechanics

`TeaqlEntityDefaults<Derived>` supplies the independent metadata protocol:
`field_layout(resource)` returns a successful empty optional, and
`register_into(store)` preserves the previous descriptor registration body.
EntityDefaults inherits this metadata default. Generated factories keep their
existing checked layout implementations and hide the default. A metadata-only
fixture satisfies TeaqlEntity without satisfying Entity; no persistence or
mutation responsibility is folded into metadata.

The core `runtime_state_any()` hook now exposes the actual default empty borrowed
AnyRef, matching the source hidden hook. This keeps a core-neutral extension point
without including runtime state types in core. AnyRef is a non-owning borrow and
must not outlive its pointee. The earlier C++ observer inferred default absence
through the structural LedgerEntity protocol; the current observer calls the
actual core default. Generated/runtime typed-ledger dispatch and generated
nonempty overrides retain their separate qualification scope. This checkpoint
does not claim that every generated runtime-state bridge has migrated.

Audited stores the existing distinct MutationIntent instead of a plain string.
Construction delegates to its validated factory; getters and consuming access
validate that intent, preserving exact untrimmed text and rejecting moved-from
C++ wrappers. Applying the reason still calls the entity's set_comment once on
consumption. Allocator-aware destination copies retain their existing payload
capability constraints rather than requiring ordinary Clone/copy/assignment.
C++ construction continues to validate before moving the referenced argument;
failure does not promise Rust panic/drop timing. Explicit Context teardown and
ordinary RAII ownership remain the established C++ reclamation boundaries.

BoxedRelations adds a distinct consuming `inject_into_json_values(record, loaded)`
bridge. It forwards an underlying override, including the same borrowed load state;
otherwise it uses the original trait-default mutation injection. One- and
two-level boxes preserve descriptor/decode delegation and failures. The mutation
bridge itself remains unchanged. No Context parameter, extra owner, cache or
implicit SQL is introduced.

The remaining generated integration must emit a thin borrowed-root adapter and
qualify automatic dynamic-save dispatch, wrong/absent carrier handling and
Context retention without introducing another owner. Existing typed-ledger
compatibility is preserved; adding the core default is not acceptance of every
generated override or runtime caller. This remains an explicit G13 obligation.

## Include dependency repair

Installing typed MutationIntent in Entity exposed the actual cycle
`entity -> request_intent -> debug -> list -> entity`. The first complete build
failed before execution and is retained in the initial native receipt and
[include-cycle archive](parity/entity_source_include_cycle_initial_5_0_6.json.gz).

The private [intent data header](../teaql_core/include/teaql/core/detail/request_intent.hpp)
now contains the unchanged data/validation bodies. The public
[request_intent.hpp](../teaql_core/include/teaql/core/request_intent.hpp) retains
its existing formatter entry point and all original inline Debug bodies. Entity
includes only the data header. The separation adds no runtime implementation,
allocation, dependency or second set of intent types; all qualified names remain
the same. A byte-preservation review separates this include-only repair from the
actual Audited/metadata/boxed changes.

## Complete source responsibility mapping

| ID | Original responsibility | Actual evidence and C++ adaptation |
| --- | --- | --- |
| EN01 | TeaqlEntity identity/descriptor, default field layout and overridable registration; descriptor store and macro dispatch | The new default-none call and metadata-only native fixture supplement 42 unchanged actual registration observations. The default body moves unchanged to the metadata base; overrides retain order and default registration avoids recursion. C++ register_entities replaces the source macro without dropping dispatch. |
| EN02 | EntityError fields, constructor, Clone/equality, Display/Debug and empty Error source | Existing 289 actual inputs retain 5,202 complete formatting strings; 102 default/error sequences include source-none controls. Context copies retain owned diagnostics. Error remains a typed payload with a nonthrowing empty cause rather than requiring exception inheritance. |
| EN03 | Entity load/dirty/new/delete/comment/original defaults and checker/on_loaded hooks | Existing complete default sequences and ownership contracts retain loaded-by-default compatibility, no-op setters, absent dirty/original/comment state and default context-aware materialization. Successful decode invokes on_loaded; failure retains EntityError. No mutation or query is invented by default hooks. |
| EN04 | Readonly types/nullable reads/presence, dynamic carrier capabilities, erased runtime bridge and default mutation/install failures | Three new actual-source cases directly compare every default, exact errors and nonmutation, including empty/Unicode/NUL codes and a private payload canary. The framework install rejects even valid supplied metadata/carriers when the default capability is absent. Core AnyRef remains a borrowed optional; runtime typed-ledger callers stay separate. |
| EN05 | Consuming into_values, distinct into_json_values, borrowed_json precedence and presentation sanitization | The existing 200-case layout/traversal/presentation corpus includes 14 exact consuming/borrowed presentations. Persistent # values, absence/NULL and exact five-key removal retain the separately accepted carrier evidence. Presentation does not manufacture mutation authority. |
| EN06 | Audited construction, private MutationIntent, immutable/mutable access, reason getter and consuming annotation | The new matrix checks all 25 Rust White_Space characters plus empty text, nearby non-whitespace, preserved padding, NUL and long reason text. MutationIntent supplies exact source failure text. All four moved-from access guards and explicit Global reason/payload retention supplement unchanged original non-Clone calls. |
| EN07 | BaseEntityData fields/default/builders, typed dynamic getters, insert/remove returns, full map conversion and parsing | The 1,481 unchanged actual public observations preserve all Value/TypedNull kinds, numeric role bounds, error precedence, Some(Value Debug) errors and complete map/Clone results. Dynamic id/version keys may overwrite write-map roles without changing stored scalar getters. |
| EN08 | BaseEntity delegation, distinct identity Value and signed version protocols | All 102 unchanged default/dynamic sequences retain setters, six typed conversions, optional prior values, sparse defaults and full identity/version ranges. BaseEntity, IdentifiableEntity and VersionedEntity remain independent concepts. |
| EN09 | Four TeaqlBoxedRelations operations and all Box delegations | The new 168-case matrix crosses default/override, one/two boxes, seven optional loaded-state shapes, successful/failing decode and empty/long/Unicode/NUL payloads. It compares descriptor fields, exact errors, unchanged seeded entries and separate mutation/presentation records. Native nested Global copies read long payloads after actual User deletion. |
| EN10 | EntityJsonTraversal Default/enter/leave, depth cap, rejected-entry push, unwind and sibling revisits | The unchanged 42 traversal inputs exercise depth 127/128/129/255, type/address pairs, empty leave and repeated siblings. Context allocation replaces a process-wide or static visited set; entity ledgers remain untouched. |
| EN11 | Generic consuming defaults without extra Clone and explicit destination ownership | The complete original non-Clone Entity/Audited/boxed program runs byte-for-byte in the exact-dependency workspace. Native ownership/registration/Debug/presentation contracts disable default PMR and retain explicit Global copies after actual deletion. Same-domain moves do not require assignment. |
| EN12 | Presentation path is separate from loading, mutation and protected execution | New core observers install no SQL provider and perform no I/O. The accepted dynamic carrier and presentation evidence preserves readonly/durable separation; generated Order and JSON hydration regressions exercise the unchanged runtime layers. Core source acceptance does not accept their entire callers or model matrices. |

## Qualification scope

The new 230-case corpus contains one metadata, 58 audit, 168 boxed and three
modern-default observations. Five retained whole corpora add 2,114 observations:
registration 42, BaseEntityData 1,481, Debug 289, defaults 102 and shape/presentation
200. All six families execute the actual original source with registry versions
and checksums from the frozen reference lock. Historical inputs/expected outputs
remain unchanged. Only local observer entry points are added to the existing
isolated workspace; the original non-Clone driver is copied byte-for-byte.

Native/imported gates include complete registration, parsing, Debug, non-Clone,
default, new protocol and presentation bodies, JSON hydration, request intent and
the existing generated Order example/import contract. Targeted sanitizers cover
the affected lifetime/borrow/resource boundaries. Compilation must finish before
execution; each successful profile ends with a no-work build.

Generator source, all 78 SDKs and 2,201 protected original/historical artifacts
remain unchanged. Prior Java/compiler/generation gates keep their own scope and
are not represented as newly executed gates for these kernel changes. The new
metadata class is exported through the core module and isolated lookup/catalog;
existing function bodies are not copied into a module implementation.

## Reproduction

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_entity_source_5_0_6.py \
    --profile native --record /tmp/teaql_entity_source_native_fresh.json.gz
```

Use fresh receipt paths; the same helper accepts `modules_native` and `sanitize`
when relevant. Complete current native SQ05/P7, generated runtime-state bridge
coverage, wider quoted-FK differences and the other complete source and phase
obligations remain open. No physical-device or MIPS result is added.

## Terminal acceptance

| Gate | Result | Evidence |
| --- | --- | --- |
| Native | 16/16; all six live-source families, 2,344 observations | [Successful retry](parity/entity_source_native_retry_5_0_6.json.gz) |
| Canonical imports | 16/16; the same complete live-source comparisons | [Modules](parity/entity_source_module_5_0_6.json.gz) |
| Targeted ASan/UBSan | 8/8 plus all six live-source families; leak scanning disabled | [Sanitizers](parity/entity_source_sanitize_5_0_6.json.gz) |
| Original non-Clone program | Byte-identical Entity/Audited/boxed program runs against exact reference dependencies in each controller | Full output and hashes in the profile records |
| Metadata publication | Facade, isolated and combined lookups publish TeaqlEntityDefaults | Combined import and structural output in the final receipt |
| Formatting | Format/check 1,671 handwritten C++ files | Final formatter log below |

The first static catalog check found the new name missing only in the combined
lookup; its [previous complete body](parity/entity_source_combined_lookup_initial_5_0_6.json.gz)
is preserved. Adding one using declaration leaves all existing lookup declarations
unchanged. The rebuilt import entry passes separately from the earlier 16-check
module selection. No failed record is overwritten or relabeled.

The [implementation review](parity/entity_source_implementation_review_5_0_6.json.gz)
binds the entire original source, scoped production/export diffs, unchanged
runtime caller, intent body preservation and protected generated inputs. The
[previous disposition archive](parity/entity_source_previous_disposition_5_0_6.json.gz)
retains all earlier entries/documents. The
[final receipt](parity/entity_source_acceptance_5_0_6.json.gz) binds current
source, code, terminal profiles and structural results.

Current source status is 21/103 and native registry 732. Historical 718/721 remains
a historical failed full-suite epoch. Current complete SQ05/P7, generated
nonempty erased-state adapters/callers, wider quoted-FK 5 and the remaining
complete phase/capability obligations remain open. No published-package, Java,
full compiler/generation, MIPS or physical-device execution is added here.
