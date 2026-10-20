# Complete evaluation source review: Rust 5.0.6

Status: complete source accepted with the explicit ownership/diagnostic
adaptations below. Native **6/6**, modules **6/6** and targeted ASan/UBSan
**4/4** pass. Source dispositions advance to **20 accepted / 104 pending**. This review covers all of
`teaql-core/src/eval.rs`, including the runtime-only sixth LoadState variant.
It does not qualify all generated E/model, SQL/provider, G13 or P7 obligations.
The reference is Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Missing Indexed diagnostics and the precise repair

The original LoadState derives Debug for every variant. Its Indexed variant
contains the complete LoadedSnapshot diagnostic payload. C++ instead passed
Indexed through its named-field formatter and printed `Indexed([])`, discarding
layout, bits, overflow, selected names and dynamic-property schema.

The [pre-repair witness](parity/eval_indexed_initial_5_0_6.json.gz) retains all
original/native outputs, complete prior C++ header/probe bodies and successful
terminal build logs. All **18/18** scenarios differ only in the initial/final
snapshot-wrapper checks. Their state transition outputs already match. The new
native lifetime contract also fails: formatting a copied Indexed handle after
actual UserContext deletion returns the empty placeholder rather than validating
the expired snapshot borrow.

The repair adds one Indexed branch to the existing LoadState debug function.
It formats an `Indexed` tuple through the existing LoadedSnapshot formatter and
accesses the snapshot through its checked borrow. Every other evaluation,
mutation, conversion, allocator and codec body remains unchanged. This is raw
framework diagnostic output; no SQL log privacy projection or entity value is
added to a logging sink. No runtime dependency or public method is added.

## Complete source responsibility mapping

| ID | Original responsibility | Current C++ evidence |
| --- | --- | --- |
| EV01 | Six LoadState variants, Default, Clone, equality and variant distinction | Existing 1,136 actual eval observations retain defaults, all non-indexed variants and the complete 23-by-23 equality matrix. The new eighteen scenarios include all six variants, checked clone equality and private original state. Indexed geometry/equality remains covered by the separate accepted load-state source. |
| EV02 | `is_loaded` for all six representations | New complete observations retain fixed/member/physical aliases, overflow positions 64/65/129, relation markers, dynamic names and unknown fields at every step. Earlier snapshot/carrier observations supplement these lookups without merging named/default variants. |
| EV03 | `mark_loaded` and `mark_unloaded` for every variant | Eighteen original/native scenarios execute all 180 ordered mutation steps, preserving NotLoaded-to-PartialCompact conversion, compact/set deduplication, shared-column detachment, FullyLoaded no-op/error behavior and indexed unknown-field errors. Full step states/errors and untouched original clones compare. |
| EV04 | `into_indexed`, matching pointer reuse and foreign layout refusal | New cases convert every final variant and distinguish foreign entity/layout acceptance for named states from refusal for an already indexed state. Existing full snapshot and initialized-cache comparisons retain same-layout identity, projection pooling and revision-specific masks. These implementations remain unchanged. |
| EV05 | Derived serialization/deserialization and runtime-only Indexed exclusion | Existing actual eval observations retain valid/invalid tagged JSON, duplicate/variant rules and generic result payloads; all new indexed summaries require the precise source serialization refusal. Native generic codecs propagate typed payload errors and never call a payload codec for NULL or NotLoaded. Non-JSON third-party codec details and serde error text are not wrapper guarantees. |
| EV06 | Complete LoadState Debug, including Indexed payload | New source/native observers retain compact, pretty, lower-hex and upper-hex/pretty output. For Indexed, each full result must wrap its complete actual snapshot representation at the right tuple indentation. The original uses an independent derived enum wrapper; C++ delegates to the existing snapshot writer. The original empty-placeholder failure and actual deletion controls prove the missing branch. |
| EV07 | EvalResult Value/NULL/NotLoaded, fields, conditional clone/debug and consuming `map`/`and_then` | The unchanged actual observer retains all 1,136 complete outputs, callback counts and four diagnostic modes, including empty/equal/nested paths, Unicode text and short circuits. Native tests consume move-only payloads/callbacks, relocate foreign nested results/diagnostics and retain explicit Global copies after actual deletion. |
| EV08 | Three original nested-graph tests and load-state distinction | Actual original tests execute unchanged. The existing native Company/Platform/User fixture retains complete leaf and middle failure paths and legitimate NULL. The canonical imported frontend now executes this entire native body too. |

## Diagnostic representation and ownership boundaries

Actual original and native initialized diagnostics are retained separately. They
are not byte-equivalent: Rust layouts include Mutex/Weak intern hints and
randomized maps; the established C++ representation keeps immutable Global
metadata and owner-thread User pools. Earlier indexed-state acceptance documents
that representation adaptation. This checkpoint does not normalize those raw
strings or discard snapshot fields to manufacture text equality. It verifies
the wrapper delegates every diagnostic mode to the full corresponding payload;
all stable state observations and errors compare exactly.

Partial's HashSet iteration and SmallVec/Arc storage also retain the existing
explicit C++ representation adaptations. No new implicit owner or default PMR
allocation is introduced. Indexed client deserialization remains rejected.

The new lifetime contract copies an indexed wrapper explicitly into Global,
actually deletes its original UserContext and checks both retained overflow
availability and full Global diagnostics. A non-owning copied User handle must
throw ReferenceError when formatted after deletion. All owning temporary strings and
containers are destroyed before their allocator closes; default PMR is disabled.
Only the non-owning Indexed handle is retained for the negative check: its field
vector has no buffer and its snapshot is a checked borrow, not an owning payload.

## Evidence and gate scope

The reference observer runs in the existing isolated core-primitives workspace.
Its registry versions/checksums must be a subset of the exact reference lock.
The earlier eval observer is included byte-for-byte through a new local entry;
no original algorithm, old corpus or Cargo dependency is changed. The new corpus
binds the complete original eval/load-state sources, observer, manifest and lock.

The first Rust fixture compile failed before execution because an array iterator
expression needed parentheses inside `json!`. Its complete source and diagnostic
are [preserved](parity/eval_indexed_fixture_compile_initial_5_0_6.json.gz). Fixing
the observer does not change original source behavior.

The new state matrix crosses all six starting variants with empty, one-field and
wide/aliased/dynamic selections: eighteen scenarios, each with ten mutations.
Outputs retain every state/error, original clone, indexed conversion and foreign
conversion, plus full diagnostics. Existing 1,136 eval observations and original
three tests execute through the same exact-dependency workspace.

The first post-repair native and module selections each pass **5/6**, with
both state corpora already matching. Their single remaining failure belongs to
the new lifetime fixture: it checks Global live User bytes after constructing
another UserContext. The [fixture archive](parity/eval_indexed_lifetime_fixture_initial_5_0_6.json.gz)
preserves its complete prior body and both failed receipts. The corrected test
measures reclamation immediately after deleting the original UserContext, before
creating the next one. Expired diagnostic rejection and independent Global
retention assertions remain intact; no production body is changed by this repair.

The native/module selections include the complete eval context body, both eval
corpora, snapshot contracts and initialized-cache comparison. Targeted sanitizer
checks cover evaluation, indexed diagnostics, snapshots and checked references.
Successful terminal builds precede execution; each profile ends with a no-work
build. Java/generation/compiler contracts are separate, unchanged producer/SDK
evidence; they are not rerun for this runtime diagnostic repair.

## Reproduction

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_eval_source_5_0_6.py \
    --profile native --record /tmp/teaql_eval_source_native_fresh.json.gz
```

Use a fresh record path. The helper also supports `modules_native` and `sanitize`
when those gates are needed. It checks the reference lock, builds the actual
observers, runs the original tests, waits for all selected consumers and records
full comparisons before declaring its finite scope complete.

## Terminal acceptance

| Gate | Result | Evidence |
| --- | --- | --- |
| Native | 6/6; full 1,136 retained and 18 new live-source scenarios | [Successful retry](parity/eval_source_native_retry_5_0_6.json.gz) |
| Canonical imports | 6/6; same complete actual-source comparisons | [Successful retry](parity/eval_source_module_retry_5_0_6.json.gz) |
| Targeted ASan/UBSan | 4/4; same live-source comparisons, leak scan disabled | [Sanitizers](parity/eval_source_sanitize_5_0_6.json.gz) |
| Actual original eval tests | All 3 pass in each profile controller; no ignored tests | Original terminal output in the profile receipts |
| Formatting | Format/check 1,669 handwritten C++ files | Final receipt below |
| Structure | Native registry 730; 860 canonical imported bodies | Structural evidence, not semantic acceptance |

The [implementation review](parity/eval_source_implementation_review_5_0_6.json.gz)
retains the complete 338-line source, exact six-line production diff and unchanged
adjacent code, producer, 78 SDKs and 2,201 protected artifacts. The
[previous disposition](parity/eval_source_previous_disposition_5_0_6.json.gz)
retains earlier source/capability entries. The
[final receipt](parity/eval_source_acceptance_5_0_6.json.gz) joins current hashes,
initial failed attempts and terminal successful gates. Earlier checkpoint
hashes retain their original scope; the six-line diff identifies the precise
change to the previously reviewed eval header. No prior gate is relabeled.

Source status is 20/104. Complete current native 730-test SQ05/P7 remains open;
the historical 718/721 full-suite result is unchanged. G15 quoted-FK 5 and other
complete capabilities/phases remain open. No Java/compiler/full generation gate,
MIPS, physical-device or external-provider execution is added by this checkpoint.
