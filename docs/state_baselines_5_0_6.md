# State, request-model and I18n baseline qualification

This checkpoint advances G01/G11/G13/G15/G17 and P7 against the exact Rust 5.0.6
reference in upstream.lock.json. Complete capabilities and phases remain open.

## Original calls and current harness adapters

The independent observer workspace is `tests/parity/rust_state_baselines_5_0_6`.
Its Cargo.lock retains the reference package versions, checked before offline
locked builds. Historical corpus, driver and parsing-helper files remain unchanged.

| Family | Historical inputs | Changed source/current-protocol outcomes |
| --- | ---: | ---: |
| Language/catalog/Context I18n | 4,575 | 0 |
| Language/catalog structural Debug | 60 | 0 |
| Entity identity graph and mutation state | 241 | 25 |
| DataService request/result models | 40 | 40 |
| Total | 4,916 | 65 |

All original/native observations match. Entity-state observations include lazy
ledgers, composition, graph freezing, weak/shared graph views and their declared
readback. The driver compiles the complete original entity_runtime.rs unchanged.
The current harness differs from its historical file only by importing the new
crate-root TraceScopeToken type and adjusting the shared helper's relative path.
These two textual differences are checked explicitly; runtime algorithms are
not copied or rewritten. Twenty-five original outcomes changed from the older
corpus, while the current C++ implementation already matches them.

The DataService observer reuses the existing
`rust_field_layout/src/bin/data_service_model_5_0_6.rs` harness with actual current
QueryIntent and MutationCommand/MutationRequest constructors. Its complete
structural diagnostics, clone observations and constructor errors are compared
exactly. It declares its fixed test purpose in trusted fixture code; that is not
an application fallback or permission to execute missing-intent requests. These
are current observer-protocol results, not claims that the retired driver compiles.

The first C++ attempt aborted after twelve inputs because the fixture called
value() on a rejected intent. It now returns the actual error code, field and
message before request construction, matching the source observer. The
[original process failure](parity/state_baselines_5_0_6_process_failure.json.gz)
is retained. This fixes test error reporting; no production function changes.

Separate active corpora preserve historical inputs, source/observer fingerprints
and exact outputs. Nothing is filtered or normalized. See the
[replay record](parity/state_baselines_5_0_6.json).

## Typed aggregate harness remains unresolved

The [initial compiler diagnostics](parity/state_baselines_5_0_6_initial_compiler_failures.json)
retain three kinds of historical harness failures: request-model retirement,
the missing trace-token import and an incomplete private typed-aggregate shell.
The first two have runnable adapters above.

The old typed-aggregate shell still lacks the real registry read-sidecar decoder,
request_intent state and query-scoped service/dynamic-field methods required by
the complete current helper. Its original source segments are extracted unchanged
by build.rs, but that shell does not compile. No trap, invented return value or
stub is substituted for the missing runtime responsibilities, and its 48 historical
observations are not accepted as 5.0.6 results. This is harness migration work,
not evidence that the original runtime fails. Full helper/provider/typed execution
composition remains required before this binding can pass.

## Verification and remaining integration

Native and canonical modules each pass 4/4; targeted ASan/UBSan passes 1/1 for
the repaired model fixture with leak scanning disabled. The complete source/native
matrix reproduces through both frontends. Canonical verification covers 487
consumer/fixture bodies and 1,479 exports; pinned format/check covers 1,354 files.
No MIPS/QEMU or physical-device execution is claimed.

The [terminal record](parity/state_baselines_5_0_6_terminal.json) retains these
scopes. Four recorded integration failures have selected successful follow-ups,
leaving 16 unresolved from the original full native integration. The original
472/549 result remains historical and failing; no fresh full suite is accepted.
The [static corpus inventory](parity/active_corpus_baseline_after_state_baselines_5_0_6.json)
is not semantic or complete capability acceptance. All G01-G17/P0-P7 gates retain
their partial status; generator code and default guards are unchanged.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_state_baselines_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(i18n|i18n_debug_scope|entity_runtime|data_service_model_scope)_rust_parity$'
python3 scripts/verify_state_baselines_5_0_6.py \
  --cpp-prefix teaql_module_ --build-dir build_modules_native/modules
ctest --preset modules_native --output-on-failure \
  -R '^module_(i18n|i18n_debug_scope|entity_runtime|data_service_model_scope)_rust_parity$'
ctest --preset sanitize --output-on-failure \
  -R '^data_service_model_scope_rust_parity$'
```

Build the selected C++ probes first. The replay selects only the four runnable
source observers. An all-bin historical observer build is not claimed to pass.
`--record` stages new corpora only when every declared comparison succeeds.

The later [complete typed-aggregate checkpoint](typed_aggregate_5_0_6.md) replaces the reduced shell as active evidence for that private entry point. It compiles the complete original runtime through an append-only access bridge, preserves the retired shell/compiler record, and replays historical inputs plus separately declared current-protocol controls.
