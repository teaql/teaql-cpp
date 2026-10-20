# SQL request protocol qualification against Rust 5.0.6

This checkpoint advances G09/G13/G14/G17 and P7. Complete capabilities and P0-P7
remain open. The runtime reference is the exact 5.0.6 commit in upstream.lock.json.

## Current constructors and preserved historical inputs

The four runnable observers use the actual QueryIntent, MutationIntent,
QueryRequest and MutationCommand/MutationRequest contracts. Their independent
development workspace is `tests/parity/rust_sql_protocol_5_0_6`. Historical driver
files remain unchanged in `rust_values`; their retired-API compiler diagnostics
remain historical harness evidence, not runtime defects.

The baseline observer manifest from the previous checkpoint was restored byte
exactly and its existing source replay passes. New observers and new corpora do
not overwrite those earlier observations.

Every historical input is retained unchanged. All 3,473 historical cases now
produce current-constructor rejection because they lack required purpose or
mutation reason. Their new tagged `intent_error` values contain the actual source
constructor's Display result; C++ uses the corresponding fallible constructor.
This is a declared observer response envelope, not a SQL executor error or proof
that rejected operations executed. Explicit query_log_intent preflight calls
retain their historical ordering and may perform schema/compiler lookups, but
rejected query requests never open their query transport.

The C++ query, stream and default-SPI fixtures no longer supply an invented fixed
purpose or mutation reason. Valid input controls are separate copies with explicit
declarations. Mutation controls supply reasons to each typed child request; this
raw observer tree is not the graph-save descendant-reason contract. Additional
controls check null, blank, Unicode-whitespace and wrong-type intent input, plus
missing/blank mutation reasons in the default-SPI fixture. Runtime intent
validation is active with diagnostic output disabled.

| Matrix | Historical inputs | Explicit-intent controls | Negative controls | Total |
| --- | ---: | ---: | ---: | ---: |
| Executor defaults | 144 | 144 | 8 | 296 |
| Query/cache/diagnostics | 381 | 381 | 5 | 767 |
| Stream/drop/diagnostics | 880 | 880 | 5 | 1,765 |
| Mutation execution | 1,012 | 1,012 | 5 | 2,029 |
| Transaction execution | 1,012 | 1,012 | 5 | 2,029 |
| Executor backend capture | 44 | 44 | 5 | 93 |
| Actual SQLite cached groups | 0 | 8 | 0 | 8 |
| Total | 3,473 | 3,481 | 33 | 6,987 |

The valid controls compare actual original executor calls, transport observations,
counter transitions, pending/drop/error behavior, diagnostics, mutation readback
and transaction paths. They are not substitutes for the unchanged rejected inputs.
Ordinary JSON outputs are exact; there is no output normalization.

The execution observer reuses the existing byte-preserved 5.0.6 original executor
observer. Only its input-construction gate is added. Query/stream/default-SPI
adapters preserve their historical fixture provider mechanics while replacing
retired constructors. No algorithm from the runtime is copied into a provider.
See the [replay record](parity/sql_protocol_5_0_6.json).

## HAVING parameters in cached partition queries

The previous compiler repair retained GROUP BY/HAVING in ranked SQL. These new
valid-request controls exposed a second early return in `collect_select_bindings`.
The SQL and policy vectors had four slots, but cached execution supplied only
three values, dropping HAVING. The [12 initial differences](parity/sql_protocol_5_0_6_initial_gaps.json.gz)
preserve missing values in both transports and retained metadata.

The collector now visits HAVING before the partition return, matching the original
executor. Window-order bindings remain before WHERE/HAVING; ordinary-order values
remain after them. The unit regression runs the same statement twice with changed
sensitive HAVING text, requires four transport/diagnostic bindings, retains the
Masked policy and verifies one compilation for two executions.

The actual SQLite observer uses the original SqlDataServiceExecutor, SqliteDialect
and SqliteMutationExecutor with a real schema provider. Eight scenarios each run
three statements through one cache, changing the HAVING threshold without changing
the SQL shape. All 24 actual database query results, parameter values, parameter
policies and parameterized SQL match C++. This covers the real binding path rather
than only a recording transport. Native connection/provider/cache objects belong
to UserContext; the default PMR resource is disabled and Users are actually deleted.

## Terminal verification and remaining integration

Native, canonical modules and targeted ASan/UBSan each pass 8/8. All 6,987 source
observations reproduce, including through module consumers. The existing explicit
5.0.6 execution regression passes 1/1 and the previous 7,040-outcome SQL family
replay still matches. Module verification covers 465 canonical consumer/fixture
bodies and 1,479 exports; pinned format/check covers 1,354 handwritten files.
Sanitizer leak scanning is disabled.

The [terminal record](parity/sql_protocol_5_0_6_terminal.json) retains each scope.
Six recorded historical-corpus failures now have selected successful follow-ups,
leaving 37 unresolved failures from the original full integration. The fresh
[static inventory](parity/active_corpus_baseline_after_sql_protocol_5_0_6.json)
does not establish complete-suite acceptance. No fresh full native, MIPS/QEMU or
physical-device run is claimed. Producer sources and default generation guards
are unchanged. Broad model/configuration, policy/privacy, provider composition,
lifecycle and phase exit criteria remain separate requirements.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_sql_protocol_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(executor_defaults_scope_rust_parity|sql_(query_rust_parity|stream_rust_parity|execution_rust_parity|transaction_rust_parity|executor_kind_rust_parity|cached_partition_rust_parity|query_executor_contracts))$'
python3 scripts/verify_sql_protocol_5_0_6.py \
  --cpp-prefix teaql_module_ --build-dir build_modules_native/modules
ctest --preset modules_native --output-on-failure \
  -R '^module_(executor_defaults_scope_rust_parity|sql_(query_rust_parity|stream_rust_parity|execution_rust_parity|transaction_rust_parity|executor_kind_rust_parity|cached_partition_rust_parity|query_executor_contracts))$'
ctest --preset sanitize --output-on-failure \
  -R '^(executor_defaults_scope_rust_parity|sql_(query_rust_parity|stream_rust_parity|execution_rust_parity|transaction_rust_parity|executor_kind_rust_parity|cached_partition_rust_parity|query_executor_contracts))$'
```

Build those selected C++ targets before replay. The verifier builds current source
observers with offline locked dependencies. `--record` stages separate corpus files
only after every declared comparison succeeds. The original historical adapters
retain their retired syntax; use the new observer workspace for current execution.
