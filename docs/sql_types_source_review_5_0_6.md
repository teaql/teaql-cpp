# Complete SQL types and SQLite renderer source review

Status: ST01-ST15 and SR01-SR06 are accepted for both complete named
source responsibilities within the SQLite profile and explicit C++ ownership/
interface adaptations. Other backend branches remain excluded; whole
capabilities and P0-P7 remain open. Reference: Rust
5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The scope is the complete 332-line `teaql-sql/src/types.rs` and 118-line
`src/log_rendering_tests.rs`, with the standing SQLite-only backend exclusions.
Bindings have their separate [SB/BT acceptance](sql_bindings_source_review_5_0_6.md).
The [older model review](sql_model_api_contract.md) retains its historical 5.0.5
evidence and does not substitute for current source observations.

## Complete responsibility matrix

| ID | Original responsibility | Preserved native contract and evidence |
| --- | --- | --- |
| ST01 | Database discriminators | PostgreSql/Sqlite/MySql remain distinct Copy/Clone/Debug/PartialEq/Eq values. Valid diagnostics preserve all three names; execution/literal/lexer implementations remain SQLite-only. Unsupported native kinds fail closed, not as extra providers. |
| ST02 | Complete CompiledQuery state | Keep independent SqlLogContext, parameterized SQL, ordered Value parameters and optional comment. Explicit destination copies retain all nested strings/values/private state. PartialEq includes every field and keeps Value NaN non-reflexivity; it is not Eq or a source Default/serde protocol. |
| ST03 | Comment preparation | Absent/empty comments return original SQL. Nonempty comments preserve all bytes and replace every non-overlapping closing delimiter with `* /` before prefixing SQL. Whitespace, control/NUL/Unicode and repeated delimiters remain distinct; source SQL/comment objects are unchanged. |
| ST04 | Diagnostic SQL and omission | debug_sql uses sql_with_comment and the shared literal/lexer functions. Any rendering error returns the exact omission marker/reason and never appends original SQL. Raw diagnostic literals are not privacy projection, provider execution or a permission to reveal bindings. |
| ST05 | Callback boundary | Rendering supplies only positional indices, in order, and propagates the first literal error unchanged. A malformed/unsupported placeholder rejects before its callback. No template text, names or inferred policy are passed to the literal callback. Observers retain complete callback sequences even on errors. |
| ST06 | SQLite quotes | Single/double/backtick delimiters support doubled matching delimiters; bracket identifiers end at the first closing bracket. Unterminated forms fail. Question marks and named markers inside quotes do not consume parameters. UTF-8 source strings and actual NUL bytes remain intact. Invalid C++ byte strings outside Rust's string domain are not a new guarantee. |
| ST07 | SQLite comments | Line comments stop at CR/LF; nested block comments count opens/closes and reject unfinished depth. Quoted/commented placeholders remain literal. The body neither executes SQL nor treats arbitrary text as authorization. |
| ST08 | Strict positional accounting | Ordinary `?` consumes exactly the next value. Numbered `?digits`, named `:`, `@`, `$`, missing values and unused values have independent exact errors. Native positional counting replaces the source used-vector because SQLite's accepted placeholders use each sequential index once; completion has used every parameter iff the final count equals the supplied count. Callback/error order remains unchanged. |
| ST09 | Finiteness | Reject non-finite F64 and recursively nested List values. Object/JSON branches do not gain an invented recursive finiteness policy; they use the independent source JSON projection contract. Object-contained NaN and a List containing an Object are observed separately from a List containing a non-finite scalar. |
| ST10 | Every SQLite literal variant | Null/TypedNull render NULL; Boolean renders TRUE/FALSE; integer/float/Decimal formatting retains source distinctions. Text/JSON/Object escape each apostrophe; Date is quoted and Timestamp remains signed milliseconds. List preserves ordered recursive literals and empty-list syntax. No driver binding is changed by diagnostic formatting. |
| ST11 | All twelve error variants | UnknownEntity, UnknownField, EmptyInList, MissingIdProperty, MissingVersionProperty, EmptyMutation, InvalidRecoverVersion, UnsupportedSchemaType, InvalidSchemaShape, SchemaIdentifierTooLong, InvalidFunctionArguments and InvalidSubQueryOperator retain all active payloads and declared raw Debug shapes. Flattened native inactive fields do not enter equality. |
| ST12 | Error derives and protocol | Source Debug/Clone/PartialEq/Eq, complete Display strings and the default absent Error source remain. Native errors expose typed data, scoped copies, to_string and source; this is a value/error protocol adaptation rather than a Rust trait-object ABI. The twelve-by-twelve matrix and independent active/inactive field tests remain complete. |
| ST13 | Native-width boundaries | Byte counts/maxima are usize in the reference and size_t natively. New native64 cases cover zero, normal limits and u64/usize maximum across all ten DataTypes and empty/NUL/Unicode/long error payloads. Their comparators are registered only on 64-bit builds; no new MIPS observation follows. |
| ST14 | Lifetime and authority | Complete unchanged model contracts retain private context/projection state, explicit Global copies after actual User deletion, nested allocators, checked reference expiry and default-PMR-disabled bodies. Callback output and errors retain their existing borrow/scoped allocation contracts. Neither presentation values nor successful rendering authorize persistence. |
| ST15 | Whole source qualification | Current exact-lock original tests/observers, retained and new corpus bytes, canonical full imported consumers and installed contracts are required. An inventory or a few successful statements does not accept full error/literal/lexer responsibilities or whole runtime publication. |

## Original rendering tests and backend exclusions

All six original tests execute unchanged against the original Rust library.
C++ preserves every SQLite branch in the five tests containing SQLite behavior:

| Original test | SQLite responsibility and explicit exclusion |
| --- | --- |
| projected_values_share_the_debug_renderer | Exact escaped literal, independent safe callback result and unchanged source value for SQLite. The other two backend loop branches are excluded. |
| quotes_comments_and_unicode_do_not_consume_bindings | Exact quote/comment/Unicode output plus bracket identifiers for SQLite. MySQL loop iteration is excluded. |
| postgres_repeated_bindings_dollar_quotes_and_nested_comments | Entire PostgreSQL-specific repeated-number/dollar-quote contract is excluded. Shared nested-comment mechanics have their own actual SQLite observations. |
| incomplete_or_ambiguous_bindings_fail_closed | All eight original SQLite cases are preserved. PostgreSQL/MySQL-specific cases are excluded. |
| renderer_propagates_projection_failure_without_echoing_input | Callback error identity, omission marker and absence of private original SQL are preserved completely. |
| null_arrays_and_finite_numbers_follow_dialect_rules | Original SQLite NULL assertion is preserved. PostgreSQL array and MySQL backslash behavior are excluded. Additional native non-finite/list rejection asserts the actual original SQLite function behavior through direct comparisons. |

The retained discriminators and executed original non-SQLite test branches do
not imply native support for those providers. The C++ port's unsupported-kind
assertions verify the standing edge boundary separately.

## Actual-source observations

The isolated `rust_sql_types_source_5_0_6` observer calls the unchanged retained
public type/renderer and complete model drivers. Exact reference registry
versions/checksums are required. The previous 1,538 type/renderer and forty
complete model observations remain immutable.

The added 552 type cases contain twenty-four literal values, 432 SQL/comment/
parameter/callback-failure cases and 96 complete error payloads. Nested Object
versus List non-finiteness, typed NULL, signed timestamp extremes, escape/control/
Unicode/NUL bytes, malformed quotes/comments and named/numbered placeholders
remain independent. Eighty added model cases cross ten DataTypes and eight
payload/control states, retaining all error Debug/Display/cause/equality fields,
complete query/context state and actual maximum usize values.

Both native observers call the production functions; the callback only records
indices and optionally returns the authored literal error. It does not implement
lexer, quoting, omission, comparison or error-display algorithms. New module
consumers execute the complete existing bodies. No C++ implementation or existing
test/observer body changes are planned for this qualification.

## Acceptance boundary

The [terminal acceptance](parity/sql_types_source_acceptance_5_0_6.json.gz)
joins the [exact original/native/imported replay](parity/sql_types_source_final_5_0_6.json.gz)
and [installed selections](parity/sql_types_source_installed_5_0_6.json.gz).
All required compiler/linker/test processes are terminal and successful.

| Gate | Completed evidence |
| --- | --- |
| Actual original tests | All six unchanged original tests pass. C++ accepts all SQLite branches in the five relevant tests; the whole PostgreSQL-only case and other backend branches remain explicitly excluded. |
| Retained observations | All 1,538 type/renderer and forty complete model observations reproduce on both C++ frontends without changing their corpora. |
| New boundaries | All 552 literal/lexer/callback/error and eighty complete model outcomes agree, including native64 maxima. |
| Exact source replay | All 2,210 observations per original/native/imported frontend reproduce again, with byte-identical retained/new corpora and no result/error normalization. |
| Installed contracts | Native 6/6 and imported 6/6, exact case sets, no skips and no-work builds before/after execution. |
| Canonical modules | 946 complete consumer/fixture bodies verify, including two new permanent consumers. |
| C++ preservation | No handwritten C++ changes: all production, contract and observer bytes remain unchanged. The preceding pinned 1,684-file format/check remains separate unchanged evidence. |
| Artifact preservation | Six native archives, all 78 SDKs/3,457 files and 2,201 protected artifacts retain their hashes. |

Exactly `teaql-sql/src/types.rs` and `src/log_rendering_tests.rs` advance to
accepted within the documented SQLite profile. Source status is **50 accepted /
72 pending / 2 excluded_external**. All seventeen capability statuses remain
unchanged: eleven qualified and six partial. Two new native64 comparisons bring
discovery to 751; this is not a whole-suite pass. Historical 735/736 and 718/721
failed full epochs retain their original outcomes.

No runtime/generator implementation or generated SDK body changed. Full SQL
dialect/executor/provider, protected runtime callers, G13/G14/G17/P0-P7 and
unanswered source-behavior decisions remain independent. No new Java/generation,
full-suite, sanitizer/MIPS, external-provider, GPU or physical-device result is
claimed. This does not accept non-SQLite rendering or native32 execution of the
new width matrices.

## Reproduce

From the supported native64 toolchain checkout:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_original_sql_rendering_tests teaql_sql_types_probe \
    teaql_sql_model_api_tests teaql_sql_model_api_probe
cmake --preset modules_native
cmake --build --preset modules_native --parallel 2 --target \
    teaql_module_original_sql_rendering_tests teaql_module_sql_types_probe \
    teaql_module_sql_model_api_tests teaql_module_sql_model_api_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_sql_types_source_5_0_6.py \
    --module-build-dir build_modules_native --record "$replay_directory/source.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(original_sql_log_rendering_contracts|sql_types_rust_parity|sql_model_api_contracts|sql_model_api_rust_parity|sql_types_boundaries_rust_parity|sql_type_models_boundaries_rust_parity)$'
ctest --preset modules_native --no-tests=error --output-on-failure \
    -R '^module_(original_sql_log_rendering_contracts|sql_types_rust_parity|sql_model_api_contracts|sql_model_api_rust_parity|sql_types_boundaries_rust_parity|sql_type_models_boundaries_rust_parity)$'
```

The reproducer validates reference identity and exact dependency versions/
checksums, waits for successful original builds, executes the authentic tests
and compares every complete output and corpus byte. Installed receipts retain
commands, complete output, JUnit case sets and stable builds. Original non-SQLite
results remain reference observations rather than native provider evidence.
