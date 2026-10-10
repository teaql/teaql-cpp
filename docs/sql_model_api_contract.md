# SQL binding and model responsibility review

## Source acceptance, 2026-10-08

The binding/type source group is accepted after complete 102/332-line public/
private/default/trait/protocol/ownership review and final current native 314/314
in 74.66 seconds. The post-test build reports no work and 314 tests remain
registered. Fresh named sanitizer/MIPS model/renderer selections pass 5/5;
source observations, active error comparisons, callbacks, consuming inputs,
allocation rollback and actual User deletion cover the documented SQLite profile.
Non-SQLite renderer/executor implementations remain excluded by the authorized
scope while their discriminators are preserved. This source acceptance does not
accept the remaining executor/provider or runtime safe-log layers. Earlier
partial/pending paragraphs below are historical checkpoints.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-sql/src/bindings.rs` and `teaql-sql/src/types.rs`. This source group is
accepted within its documented SQLite and Context ownership profile. Database discriminators remain separate from provider support; SQLite
is the only supported executor/provider and literal-rendering implementation.
No PostgreSQL/MySQL service is added by this review.

## Responsibilities

| Original item | Mapping and acceptance requirement |
| --- | --- |
| SqlBindings new/Default | Explicit-domain empty values/policies, absent scope, trusted/generated SQL by default. No hidden Context or thread-local binding state. |
| push | Append one value and the current scope policy, defaulting to Unknown; keep values/policies aligned on C++ allocation failure. Review consuming input transfer separately from borrowed deep copying. |
| push_field | Resolve actual property metadata and append its own policy independently of the outer scalar scope. |
| mark_untrusted_sql | Monotonic untrusted flag; log_context cannot claim generated SQL afterward. |
| log_context | Independent destination-owned SqlLogContext; lower-case discriminator, ordered policy copies and default remaining fields. |
| into_values | Consuming values extraction; policies are not execution bindings. No implicit cross-domain retention. |
| Deref to Value slice | Const span, iteration, indexing and size retain ordered borrowed access. Views expire with the owner. |
| Clone and Debug | Explicit destination copies must include private scope/trust state as well as values/policies. Derived raw Debug must retain all four original fields. Source defines neither Eq nor serde. |
| field_policy | Unknown property first; credential property/column names override unknown-mask metadata; declared mask membership precedes Plain. |
| combine | None identity, then Credential, Unknown, Masked, Plain precedence. |
| expression_policy | Scalar columns/functions/binary/between/null predicates combine provenance; subquery left expression is distinct from its entity scope; value/logical roots do not taint siblings. |
| DatabaseKind | Three distinct scalar discriminators with Copy/Clone/Debug/PartialEq/Eq; non-SQLite implementation support remains excluded. |
| CompiledQuery fields | Independent log_context, SQL string, ordered Value bindings and optional comment. Explicit-domain Clone and structural PartialEq; source has no Default/serde derive. |
| sql_with_comment | Keep absent/empty comments, preserve original SQL and replace every closing comment delimiter in nonempty comments. |
| debug_sql | Render raw diagnostic literals through strict placeholder accounting; failures produce the original omission marker. Safe runtime logs require independent privacy projection. |
| render_sql_with | Callback order/failure, SQLite quoting/comment recognition, numbered/named binding rejection and missing/unused binding accounting. Never execute the interpolated diagnostic as provider SQL. |
| render_sql_value | Preserve every Value variant's SQLite literal, recursive list finiteness and quoting; original JSON/object projection and typed-null behavior remain distinct. |
| SqlCompileError | Twelve variants, active payloads, scoped Clone, variant-sensitive PartialEq/Eq, Debug/Display and default Error cause. Flattened C++ payload storage is not permission to change variant equality. |

## Complete SQLite-profile body review

The review reads all 102 lines of bindings.rs and all 332 lines of types.rs,
including private provenance, finiteness, literal quoting and lexer helpers.
Bindings preserve missing-property-first policy selection, credential precedence,
None identity in scope combination, scalar expression taint and logical/subquery
isolation. C++ policy-first reservation plus rollback is the explicit recoverable
allocation adaptation; successful consuming and borrowed paths preserve source
values/policies and do not introduce ambient state.

The SQLite renderer recognizes quoted strings/identifiers, bracketed names,
line comments and nested block comments before positional placeholders. Numbered
and named bindings reject before a callback; missing/unused checks and callback
failure retain their source boundaries. SQLite has one-use positional indices,
so its C++ positional counter replaces the source used-vector without changing
callback order or strict accounting. UTF-8 input bytes are preserved; source Rust
strings are valid UTF-8, while C++ byte strings outside that domain are not a
new source guarantee. List finiteness descends through lists as in the source;
object/JSON projection retains the independent Value JSON contract.

All four CompiledQuery fields, comment-delimiter replacement, omission diagnostics,
all twelve active error payloads and their empty default cause are reviewed.
Projection origin equality now survives explicit allocator-aware retention;
that repair is verified separately in the data-service lifecycle review.
Database discriminators remain present, while PostgreSQL/MySQL execution and
literal/lexer profiles are excluded by the authorized SQLite-only scope. Neither
model defines serde, and CompiledQuery does not acquire a source Eq or Default
claim from native conveniences. The current whole-project consumer refresh is
still required before accepting this reviewed source group.

## Final-header model and renderer cross-platform checkpoint

Current selected sanitizer and MIPS builds each finish all 23 tasks before tests
run. The five-check selection passes ASan/UBSan with leak scanning disabled 5/5
in 15.91 seconds and MIPS/QEMU 5/5 in 8.26 seconds. It covers model ownership/
active-variant contracts, complete model diagnostics, the 1,538 rendering/binding
observations, callback/error contracts and the direct original SQLite renderer
ports. The source body reading above is complete; current whole-project native
integration remains running. Earlier full-suite totals predate the late headers
and are not reused as acceptance of the current consumer state.

## Reproduced gaps and current repairs

A direct public-facade compile check on 2026-10-08 fails for
`SqlCompileError::source()`, `write_debug(SqlBindings)` and
`write_debug(CompiledQuery)`. The source has an empty Error implementation and
derived Debug on both models. The public-facade compile check now succeeds after
adding the nonthrowing empty source and complete model writers. Nested log Debug
includes all original fields, with the original private projection placeholder
and redaction count; private payload text is not printed.

SqlCompileError equality now compares only the tag and that variant's active
payload. Native contracts perturb every inactive flattened field, then change an
active payload separately. All twelve variants retain their cause/Clone/Debug/
Display responsibilities. Source has no inactive payload fields to compare.

SqlBindings now has consuming push/push_field overloads alongside the explicit
borrowed-copy path. Same-owner strings and nested lists transfer their buffers;
foreign inputs deep-copy into the binding destination and preserve the source.
The shared append kernel restores policy/value alignment on failure. Failure
injection covers policy allocation, value-vector allocation and foreign text
copying without consuming the source.

## Original-call and lifetime evidence

Forty original-call inputs exercise all Value alternatives, five optional scalar
scopes, trusted/untrusted text, independent field provenance, binding Clone and
slice observations, complete nested CompiledQuery/log state, all twelve errors
and all database discriminators. Every derived diagnostic is observed in nine
formatter modes: 10,710 raw diagnostic observations, together with 5,760 pairwise
error comparisons. The Rust fixture compiles the complete unchanged bindings.rs
to access its compiler-only scope, and calls public original CompiledQuery and
error methods directly. No source algorithm is copied into the oracle.

The corpus is deterministically compressed to approximately 154 KB. Named
checks are sql_model_api_contracts and sql_model_api_rust_parity. Fresh native,
sanitizer and MIPS builds use independent build-sql-model-review directories,
avoiding the already-running shared consumer builds. Both named checks pass on
native (2/2, 7.65 seconds), ASan/UBSan with leak detection (2/2, 28.35 seconds)
and OpenWrt MIPS/QEMU (2/2, 78.37 seconds). Native and MIPS probes also match all
40 inputs against live original Rust. An initial observation-probe compilation
fails from passing an allocator twice; the fixture is corrected and rebuilt.
That failed compilation is not passing evidence.

The existing 1,538 SQLite literal/rendering/binding inputs remain unchanged and
match live original Rust on native and MIPS. Native dialect scope/failure
contracts also pass. The corresponding MIPS rendering/dialect selection passes
2/2 (27.54 seconds), and ASan/UBSan with leak detection passes 2/2 (4.25 seconds).
After refreshing every affected selected executable, the four-check combined
selection passes native 4/4 (15.94 seconds), ASan/UBSan with leak detection 4/4
(29.59 seconds), and MIPS/QEMU 4/4 (107.29 seconds). The cross probe is ELF32,
big endian, O32 and MIPS32r2, built for 74Kc with soft float. QEMU execution does
not prove physical-router execution. Complete whole-project consumer and
composition acceptance remain required.

Native lifetime checks disable default PMR. They preserve private scope and
untrusted provenance during explicit Global copies, verify nested private state
allocator rebinding, reject checked references after User close, delete User,
assert complete source-block reclamation and compare all retained diagnostics.
Same-owner private projection copies preserve identity. Cross-domain retention
deep-copies its concrete allocator-aware payload rather than extending User
lifetime with Rust Arc ownership; this is the explicit Context-owner adaptation.
Local owning views are destroyed before manual User close.

Existing SQL model/dialect contracts exercise nested scopes, subquery isolation,
exception restoration, generated-SQL provenance, callback failure and real
compiler policy composition. Provider binding tests exercise native SQLite
conversion/retention separately. These are useful evidence for their actual
scope; they do not prove every derived model capability or close Stage 3.

Next work: complete the fresh comparisons/ownership checks, review the complete
SQLite rendering and compiler/executor/log composition evidence, and refresh
all affected consumers at the final worktree state. Source-group and Stage 3
acceptance remain open.
