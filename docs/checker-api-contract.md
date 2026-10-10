# Checker production API and wire contract review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Complete checker.rs production body is reviewed through line 624; embedded tests
are independent evidence. This group remains partial until complete installed
integration and Context/typed-checker/save composition acceptance finish.

| Responsibility | Native mapping and semantic boundary |
| --- | --- |
| Object status | Exact internal field and create/update/unknown names. Recognized explicit text overrides ID fallback; missing or untyped Null ID means create, TypedNull still means update. Value conversion, mark and clear remain separate helpers. |
| CheckRule | Eight variants with exact wire IDs and derived Copy/Clone/equality/Debug. |
| LocationSegment/ObjectLocation | Independent member/index segments, root/default/Clone/equality, hash/array roots, append operations and level. Model/native paths preserve original spelling; Display uses `$` only for empty location. |
| Naming profile | CamelCase default, exact accepted model strings, SnakeCase and PascalCase. Underscore splitting and Unicode uppercase expansion follow the original, without inventing word/acronym normalization. |
| JSON pointer | Profile-render property names first, then escape `~` and `/`; numeric indices remain native-width decimal. Empty member names remain distinct from no segments. |
| CheckResult | Rule/location, independent optional input/system Value/message/entity/source path. Factories and fluent methods retain exact source fields; None differs from Some(Null), NaNs preserve PartialEq. Display prefers even an empty present message. |
| WireCheckResult | Independent rule ID, entity, original segment payloads, profile-rendered instance path, source path, input/system JSON and message. Clone/PartialEq/Debug and Serialize are separate responsibilities. |
| WireLocationSegment | Property{name} and Index{index}, source tagged camelCase JSON (`kind`, `name`/`index`), full derived struct-variant Debug, Clone/equality. |
| Wire serialization | Exact camelCase field names, all optional fields emitted as JSON null when absent, full Value-to-JSON conversion, original property names in location, and rendered names only in instancePath. No raw CheckResult protocol is invented. |
| Checker defaults | Required checks only missing/untyped Null; min/max string checks only Text and count Unicode scalar values. Location appends the offending field and returns source factory results. |
| Registry | Default/new empty ordered entity map, replacement by entity name, source Clone responsibility and checked borrowed lookup. No shared ownership extends a provider's Context. |
| Typed checker adapter | Separate typed SPI and erased Checker adapter; descriptor entity name, status detection, sparse original values, loaded/dirty field extraction, decode then typed callback then serialization. Source requires model Clone to compare before/after state. |
| Sparse merge | Only changed after-values become mutation intent. Omitted nonnullable non-ID/version values remain required when absent/null or unchanged checker defaults. TypedNull is preserved distinctly. |
| Decode failure | Restore the sparse mutation boundary after removing checker-only loaded/dirty annotations, append InvalidType with exact materialization diagnostic, and do not invoke the typed checker. |

## Context ownership and scheduling

Models, locations, wire JSON and registry maps belong to explicit allocation
scopes. Cross-domain retention deep-copies all nested strings, Values, JSON and
segments. Registry/typed-checker resources are checked borrows; old ownership
cannot be silently extended through Arc-like Context data. Native registration,
callbacks, decoding, field merge and close follow owner-thread rules. Validate
the checker borrow before taking caller values, including decode-failure paths.

The typed adapter's Clone requirement belongs to the original checker algorithm,
unlike the unrelated transaction output requirement removed earlier. It must
not be erased to make a smaller passing fixture. Decoder failure and later
mutation/governance behavior remain separate acceptance gates.

## Reproduced diagnostic gaps

Existing native writers already cover LocationSegment, ObjectLocation and
CheckResult. A public umbrella compiler fixture reproduces missing source Debug
for CheckObjectStatus, CheckRule, JsonFieldNamingProfile, WireLocationSegment
and WireCheckResult. The installed repair supplies those five complete derived
writers without changing paths, serialization or checker execution.

The original model oracle records 220 twice-agreeing actual public calls in
nine diagnostic modes. It checks constructors/fluent fields, Clone/PartialEq,
default naming, every rule/status/profile, unsupported-profile errors, Unicode
and empty/escaped paths, full-width Values/JSON, NaNs/negative zero, absent and
present null optionals, original Display, and complete serde wire output.
Existing public typed/default/Context checker matrices remain independent of
these additional model observations.

The staged native probe matches 220/220 live original cases. Its ownership
executable disables default PMR, explicitly retains CheckResult and wire model
nested storage in Global, destroys the source User, formats/serializes retained
models through a new User and rejects expired source access. Sanitizer/MIPS
and installed native/CMake validation remain separate; physical-router execution
is unverified.

## Current standalone target evidence

Native, ASan/UBSan and MIPS/QEMU finish successfully. Each model probe matches
220/220 live original cases and each default-PMR-disabled ownership/wire test
passes. ASan/UBSan disables leak scanning. Production/CMake integration and
complete checker/Context/save acceptance remain pending.

## Direct callback lifecycle follow-up, 2026-10-09

Complete source reconciliation found two native public dispatch paths outside
UserContext.check_and_fix_values: InMemoryCheckerRegistry registration calls
Checker.entity(), and TypedEntityChecker.check_and_fix invokes its typed callback
directly. Previously those calls could close the destination/calling User while
the synchronous invocation remained active. Controlled callbacks close and then
throw before any reclaimed payload is accessed: both original installed paths
exit 2, showing that close returned instead of being rejected.

The repair shares CheckerCallScope between the existing Context paths and these
two entries. Registry installation guards its destination arena before calling
entity(); the typed adapter guards the supplied Context before validating or
materializing values. No asynchronous join can complete its own callback stack,
so a reentrant close uses the existing deterministic termination contract.
The two registered child-process checks require termination; ordinary callback
exceptions still propagate, leave failed registration unchanged, release both
new guards and allow actual User deletion returning all blocks. The positive
contracts also cover Global-owned typed models and caller-selected result arenas.

The complete native consumer rebuild and focused sanitizer/MIPS rebuilds are
active; no final passing result or source acceptance is claimed yet. The earlier
369/369 native checkpoint predates this repair and remains historical evidence.
Physical-router execution remains deferred by the user.

The current MIPS/QEMU composition selection passes 31/31 in 26.46 seconds after
69 initial and six additional scope build/link tasks complete. Both new direct
callback checks and positive exception cleanup pass; pre/post-test builds report
no work. Native full and sanitizer consumer validation remain active, so no new
source acceptance is recorded yet.

Current MIPS probes additionally agree with freshly rebuilt live original Rust:
220/220 checker models, 177/177 policy models, 1,362/1,362 public checker/runtime
observations and 276/276 policy reviews. These comparisons preserve the complete
existing corpora; no normalization or fixture omissions were introduced. Native
full and sanitizer composition validation remain separate unfinished gates.

Current ASan/UBSan composition validation also passes 31/31 in 25.46 seconds,
with leak scanning disabled and no-work before/after testing. All 69 initial
and six scope build/link tasks finish successfully. Whole native integration
remains active; complete source acceptance awaits that final current checkpoint.

## Complete current source acceptance, 2026-10-09

Final native integration passes 371/371 in 210.67 seconds after its 656-task
consumer rebuild, with 418 nonempty compiler dependency records and no-work
before/after testing. All corresponding fresh sanitizer/MIPS consumer selections
pass; complete source/body/helper/input/assertion or API/default/trait/protocol/
ownership review is recorded above. The reviewed source groups in this document
are accepted within their explicit C++ ownership and SQLite scope. This does
not accept broader Context/services, graph/event/log privacy, runtime-root or
complete stages 4–6. Current full output is retained in
measurements/parity/2026-10-09-checker-direct-callback-review.
