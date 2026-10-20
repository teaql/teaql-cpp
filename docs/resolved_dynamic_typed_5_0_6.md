# Prepared selections for ordinary typed queries

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: `fetch_entities_internal<T>` in `data_service/resolved.rs` (RQ01/RQ18).
The complete resolved-service source remains `pending_review`.

## Source contract and reproduced differences

The original method optionally creates a diagnostic scope from the incoming
selection and existing request intent, then prepares the query once. Behavior
runs before RequestPolicy. It uses the resulting selection for decoding, always
fetches through `fetch_prepared_compact_owned`, and creates a fresh runtime state.
Ordinary typed reads do not use aggregation-cache lookup or storage, including
empty rows. Dynamic loading is skipped for empty rows. Unsupported carriers fail
before attempting dynamic storage for nonempty selected results.

The actual complete source executes 288 cases: incoming selection absent/present;
behavior and Policy independently keep, add or remove selection; capture and
aggregation-cache options enabled/disabled; empty/nonempty rows; and provider
success/failure. Each case performs two reads. Source invariants explicitly
check 576 provider requests, 1,152 ordered callbacks, no cache events, and scope
classification based on the original incoming selection. The readonly result
schema remains a separate observation.

The native pre-repair probe differs on 124/288 cases. Differing fields overlap:
72 cache-event observations, 72 provider-request observations, 90 classification
observations and 16 outcomes. In particular, a callback adding dynamic selection
after entry could bypass the unsupported-carrier check; enabled caching could
reuse empty results or retain rows before a decode error.

Evidence:

- `parity/resolved_dynamic_typed_source_5_0_6.json.gz`: original execution through
  the public purposed typed entry, with complete original runtime bodies.
- `parity/resolved_dynamic_typed_before_5_0_6.json.gz`: native counterexamples.
- `tests/parity/resolved_dynamic_typed_5_0_6_cases.json.gz`: immutable expectations.

The observer extends the existing query fixture with application-owned behavior
and Policy implementations. No runtime orchestration or expected results are
rewritten. Historical row/schema corpora remain unchanged.

## Runtime correction

`DynamicEntityListOperation` now handles typed queries with optional persistent
extensions, matching the original method's single preparation/decode algorithm.
It creates the query diagnostic scope only when the incoming selection exists
and request intent is not bound, preserving source classification timing. Its
provider operation always uses the compact owned path, which clears/moves trace
as required and bypasses aggregation caching. Decoding checks the prepared
query's final selection.

`EntityBasicListOperation` delegates every typed query to that shared operation;
the duplicate ordinary typed decode body is removed. Compact-row SmartList
execution retains its separate branch. Both existing classes and their module
exports remain present, as do independent base, Context, resolved-service,
executor and provider layers. The public API and generated code are unchanged.
Ordinary typed delegation adds a Context-owned operation state and moves the
existing query snapshot into it. The rvalue constructor retains the explicit
allocator and avoids another deep query copy. The original const-query
constructor remains available. Neither path adds an arena or extends Context
lifetime. The caller's original purposed query remains unchanged.

## Lifecycle and integration

The native focused checks currently pass the new 288-case replay, both previous
schema corpora (2,304 and 192), and both lifetime programs. The new program runs
with the default PMR resource disabled and exercises eight paths: selection
added/removed by Policy across unpolled deletion, pending deletion, explicit
cancellation and resumption. Provider cancellation occurs once while pending,
never for unstarted/completed work, and resumption does not rerun callbacks.
Successful decoding retains a fresh runtime state.

Joined source/native/module checks pass all 288 new observations per frontend,
with native 10/10 and modules 10/10. Both final builds report no work and canonical
module consumers pass. The selection includes original typed and aggregate
contracts, generated SQLite dynamic-field operations, dynamic schema
pending/foreign-work cleanup, and the source-backed row loader. The receipt is
`parity/resolved_dynamic_typed_move_installed_5_0_6.json.gz`.

Focused ASan/UBSan passes 4/4 with leak scanning disabled, covering the new
288-case replay, retained 192-case typed schema replay, all eight pending/close
paths and existing dynamic schema cleanup. The receipt is
`parity/resolved_dynamic_typed_move_sanitize_5_0_6.json.gz`.

The source fixture uses an unsupported carrier for the new independent replay;
positive indexed-carrier behavior is supplied by the separate existing generated
integration checks, not inferred from those 288 inputs. No new generator or
released-package verification is claimed.

```bash
source scripts/toolchains/activate.sh
CARGO_INCREMENTAL=0 python3 scripts/verify_resolved_query_schema_5_0_6.py \
    --suite dynamic \
    --record /tmp/resolved_dynamic_typed_fresh.json.gz \
    --cpp build_native_current/teaql_entity_query_probe \
    --module build_modules_native/modules/teaql_module_entity_query_probe \
    --check-gates --jobs 4
```

The new source fixture intentionally uses an unsupported entity carrier to
observe guard order and empty-result behavior without inventing storage calls.
It is not proof of all supported indexed-carrier loading, field-selection
variants, enhanced/relation compositions or storage/transaction semantics.
Those retain their own source and integration obligations. No source or phase
is promoted: source totals remain 84 accepted / 38 pending / 2 excluded and
capability totals remain 11 qualified / 6 partial.
