# Explicit TransactionScope requests on SQLite

Status: focused source qualification. All **68** complete observations match
the actual frozen Rust runtime through both C++ header and imported-module
consumers. This closes the positive raw-request evidence gap identified by the
[historical saver replay](mutation_saver_replay_5_0_6.md); complete G17, SQ07 and
P7 qualification remain open.

## Actual execution boundary

The [Rust observer](../tests/parity/rust_document_services_reference_5_0_6/src/bin/transaction_scope_intent_5_0_6.rs)
calls the public `UserContext::execute_in_transaction`, `TransactionScope::query`
and `TransactionScope::mutate` implementations from the unchanged Rust 5.0.6
checkout. The existing reference-compatible Cargo project supplies its exact
locked dependencies. No private access bridge or replacement transaction,
query, mutation, privacy or logging implementation is involved.

The [C++ observer](../tests/parity/transaction_scope_intent_probe.cpp) uses the
corresponding existing runtime, data-service, SQL executor, transport and SQLite
provider. Each observation creates an in-memory database containing one versioned
Order. Requests use actual fallible intent constructors. Transactional reads,
writes, authoritative readback, commit and rollback run through the real SQLite
connection. Final inspection reads that connection after the scope completes.

Neither observer implements audited graph Save. Raw mutation results remain
distinct from the higher-level governed Save contract.

## Finite input matrix

| Inputs | Responsibility |
| ---: | --- |
| 40 | Five query/write sequences, callback success/failure and all four independent select/mutation logging configurations |
| 12 | Empty or Unicode-whitespace comment, purpose or mutation reason, with diagnostics enabled or disabled |
| 8 | Stale expected versions below and above the stored version, before/after a successful query, with diagnostics enabled or disabled |
| 4 | Long Unicode intent, callback success/failure and enabled/disabled diagnostics |
| 4 | Literal U+0085/U+2028/U+2029 inside valid intent, with actual safe diagnostics and LF-only observation framing |

The five ordinary sequences are query; mutation; query/mutation; mutation/query;
and query/mutation/query. They preserve successful reads before a later intent
rejection and visibility of an uncommitted write before callback failure.

The original raw executor returns **zero affected rows and no snapshot** for a
stale update. A callback that accepts that result can still succeed and commit
without changing the database. C++ matches this behavior; the observer does not
invent an optimistic-lock error from the higher-level Save API.

All 68 complete result objects are compared without normalization: callback
outcome, ordered query/mutation steps, mutation snapshots, final stored rows and
every selected diagnostic field. Logging observations include complete log-context
Debug, operation, comment, purpose, audit reason, all trace-node fields, SQL,
bindings, rendered SQL and result counts/types/summaries. Wall-clock timestamps
and durations are deliberately outside this deterministic projection. This does
not qualify every diagnostic field or protected-I/O configuration.

## Provenance and verification

The [comparator](../tests/parity/compare_transaction_scope_intent_5_0_6.py) requires
the active reference commit, a clean tracked reference checkout, unchanged source
anchors and exact reference registry versions/checksums. The immutable
[source corpus](../tests/parity/transaction_scope_intent_5_0_6_cases.json.gz)
records observer, manifest, lock and source hashes. Four additional observations
use a separate [framing corpus](../tests/parity/transaction_scope_intent_framing_5_0_6_cases.json.gz),
preserving every original 64-case corpus byte. Corpus replacement with
different results is refused. Every observer must return the exact requested record count;
missing fields, missing results and extra results fail comparison.

The completed header and imported consumers pass **2/2** CTest checks in the
separate `build_modules_native` tree after a second build reports no work. Both
execute with default PMR allocation disabled. The header check in that tree is
not counted as completion of the separately running full `native` preset.
The canonical import transformation verifies **834** source/fixture bodies;
pinned formatting and its check pass for **1,654** handwritten C++ files.
See the [terminal evidence](parity/transaction_scope_intent_terminal_5_0_6.json.gz).

The [framing follow-up](parity/transaction_scope_intent_framing_terminal_5_0_6.json.gz)
retains actual Rust/C++ false parse failures from Python `splitlines()` and the
repaired **68-case** gate, header/imported **2/2**. JSON Lines uses LF; valid
Unicode separators inside JSON strings are data. This test-adapter correction
does not rewrite intent or change the runtime. The original 64-case receipt
above remains historical evidence.

```bash
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
    cargo build --jobs 2 --offline --locked \
    --manifest-path tests/parity/rust_document_services_reference_5_0_6/Cargo.toml \
    --bin transaction_scope_intent_5_0_6
cmake --preset modules_native
cmake --build --preset modules_native \
    --target teaql_transaction_scope_intent_probe teaql_module_transaction_scope_intent_probe
python3 tests/parity/compare_transaction_scope_intent_5_0_6.py \
    --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/transaction_scope_intent_5_0_6 \
    --cpp build_modules_native/teaql_transaction_scope_intent_probe
ctest --test-dir build_modules_native --output-on-failure \
    -R '^(transaction_scope_intent_rust_parity|module_transaction_scope_intent_rust_parity)$'
```

Only new observers, their corpus/comparator and test registration change. Runtime
algorithms, generated SDKs, historical raw-intent inputs and existing C++ source
bodies are preserved. No new sanitizer, MIPS or physical-device result is claimed.
The queued native registry is now **721**: the original 719, the earlier comparator
control and this raw-request check. Preserve the original build-input snapshot;
the [native integration gate](native_integration_5_0_6.md) must finish and review
the explicit test/configuration transition before running that new registry.
