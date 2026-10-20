# Request-owned intent source acceptance

Status: qualified for the complete `teaql-core/src/request_intent.rs`
responsibility under RI01-RI08. Exact-dependency source/header/imported replay,
ownership checks and module export/archive checks pass. This review does not
accept whole graph, executor, logging or G17 behavior.

## Complete source responsibilities

| ID | Source contract | C++ mapping and evidence |
| --- | --- | --- |
| RI01 | Query/Mutation request kinds, Copy/Clone/equality and Debug | The scoped intent header retains both enum values and source Debug spelling. Original error observations exercise both kinds, including deliberately unusual public error fields. |
| RI02 | Public error fields, field-based error codes, Display and default Error source | `RequestIntentError` owns its field in the destination Context; `code`, `to_string`, structural Debug, equality and empty `source()` retain the source responsibility. A `purpose` field yields `QUERY_PURPOSE_REQUIRED` even for a manually constructed Mutation error. |
| RI03 | Required optional values, Unicode blank rejection and validation order | `from_optional` checks comment before purpose and distinguishes absent, blank and valid input. The shared UTF-8 whitespace helper is independent of locale. |
| RI04 | Validated private query fields, constructors and borrowed getters | `QueryIntent::create` and `from_optional` require both fields. No default construction or Context/trace fallback can create intent. Getter results borrow the explicit owner. |
| RI05 | Validated private mutation reason and consuming conversion | `MutationIntent` requires its own reason. The source private `into_comment` returns that owned String; the C++ consuming Audited adapter applies its already validated owned text to the entity. The rest of `entity.rs` is a separate source responsibility. |
| RI06 | Exact text preservation and derived Clone/equality/Debug | Valid input is not trimmed or rewritten. Explicit destination construction replaces Rust Clone for Context-owned text. Same logical values compare equally and render source-shaped diagnostics. |
| RI07 | Two original unit tests | Both unchanged original tests execute from the frozen core crate: absent/blank/Unicode rejection and untrimmed valid intent. |
| RI08 | Explicit C++ lifetime and move adaptation | The existing complete request/trace contract checks absent defaults, moved intent rejection, nested allocation, actual UserContext deletion, explicit Global retention and independent request/command bindings with default PMR disabled. C++ moved-from states are an additional language boundary, not invented Rust inputs. |

The whole production body and both source tests are included. Request intent
does not itself authorize a database operation or validate an application's
business reason. Privacy publication, policies, canonical graph lineage and
provider orchestration remain separate responsibilities.

## Exact-dependency source replay

The historical observer workspace has registry drift. A new
[isolated observer](../tests/parity/rust_document_services_reference_5_0_6/src/bin/request_intent_reference_5_0_6.rs)
links the actual frozen runtime using the existing reference-compatible manifest
and lock. Its six historical stimulus/serialization functions remain byte-identical;
no intent validator, trace recovery or request implementation is copied.

The [replay verifier](../scripts/verify_request_intent_source_5_0_6.py) preserves
all **4,571** historical inputs and expected outputs: 486 intent/error cases,
147 trace-token cases, 2,048 query envelopes and 1,890 mutation envelopes. It adds
**84** finite controls covering every Rust Unicode White_Space character as a
blank comment, blank purpose and preserved padding; eight nearby non-whitespace
values; and literal Unicode line separators inside valid text. These additional
controls qualify lexical branches, not an unbounded model product.

Historical corpus bytes, runtime implementations and observer function bodies
must remain unchanged. Registry versions/checksums must belong to the frozen
reference lock. The verifier checks complete typed JSON results without
normalization and records current executable hashes. Valid UTF-8 text is the
Rust String domain; arbitrary invalid C++ byte strings and invalid enum casts
are not fabricated source cases.

## Imported ownership contract

The existing `request_trace_scope_tests.cpp` had no canonical imported consumer.
Registering its unchanged complete body exposed one missing export:
`detail::graph_plan_privacy`. Its declaration and implementation already existed.
One `using` declaration in the runtime module and its isolated lookup, plus the
catalog entry, publish that existing function. No function body or request/
privacy/ownership algorithm changes.

The failed import compilation is retained separately from the repaired gate.
The native header frontend does not compile either changed module/lookup file.
Nevertheless, both are part of the original broad input inventory, so its
unchanged-input guard must reject the intentional frontend transition. Preserve
that old epoch and review the exact export/configuration deltas before accepting
the next [full native gate](native_integration_5_0_6.md).

## Related JSON Lines correction

The previous raw-scope comparator used Python `splitlines()`. Actual Rust and
C++ diagnostics containing U+0085/U+2028/U+2029 reproduced a false parse failure.
The comparator now uses LF framing. Four independent source observations are
stored beside the unchanged 64-case corpus; both completed consumers compare
all **68** without changing or normalizing intent text. This repairs the test
adapter, not the runtime or historical source outputs.

## Reproduction

```bash
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
    cargo build --jobs 2 --offline --locked \
    --manifest-path tests/parity/rust_document_services_reference_5_0_6/Cargo.toml \
    --bin request_intent_reference_5_0_6
cmake --preset modules_native
cmake --build --preset modules_native --target teaql_loaded_snapshot_probe \
    teaql_request_trace_scope_tests teaql_module_loaded_snapshot_probe \
    teaql_module_request_trace_scope_tests teaql_module_contracts
python3 scripts/verify_request_intent_source_5_0_6.py \
    --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/request_intent_reference_5_0_6 \
    --build-dir build_modules_native --record /tmp/request_intent_source_fresh.json.gz
```

Use a fresh record path. The focused sanitizer gate runs only
`request_trace_scope_contracts`, with leak scanning disabled. No complete
sanitizer, MIPS, physical-device, generator or P0-P7 acceptance follows from
this source group.

## Terminal acceptance

The [exact-dependency replay](parity/request_intent_source_replay_5_0_6.json.gz)
records **4,655** complete observations each for actual Rust, headers and imports.
Both original unit tests pass. After a second build reports no work, the two
historical parity checks, both complete ownership contracts and module import
contract pass **5/5** in the separate module build tree. The focused ASan/UBSan
ownership check passes **1/1**, with leak scanning disabled.

The [acceptance receipt](parity/request_intent_acceptance_5_0_6.json.gz) preserves
failed and successful compilation, current executable/source hashes and the
exact export/configuration transition. Removing the single added `using` from
each frontend file restores its complete prior bytes. Every existing runtime
function body is unchanged. All **835** canonical bodies, **1,504** catalog
exports and seven facade archives verify; pinned formatting/check passes for
**1,654** files. The
[framing receipt](parity/transaction_scope_intent_framing_terminal_5_0_6.json.gz)
separately preserves both original parse failures and the repaired 68-case gate.

This accepts one complete active source disposition: **9 accepted / 115 pending**.
It does not accept the separate full native integration gate or promote G17.
