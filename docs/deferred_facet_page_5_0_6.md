# Deferred facet, first-result and page composition

Status: the combined original/native/module/lifetime checkpoint is verified.
G08/G09/G13/G17 and the full generator goal remain open.

The locked runtime is Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the producer is
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Missing combined evidence

Earlier checkpoints separately qualify deferred count/row pages and deferred
root/facet lists. They do not establish their composed first/list/page behavior
with actual in-memory optimization stores. This matrix exercises that remaining
G09 requirement without replacing pagination, facet, cache or provider algorithms.

The original observer calls the complete unchanged generated Order Q and Person
facet APIs. A test `TeaqlRuntime` SPI adapter selects a gated executor for both
root and facet operations. It invokes the original service methods, including
the facet trace context and relation-aggregate arguments. The executor suspends
before delegating the captured request to the actual generated SQLite executor.
Production source, generated Rust files and earlier observers remain unchanged.

Native C++ uses the existing public `EntityQuery` facade with the same generated
models and a gated actual executor resolved through UserContext. Runtime owns
count conversion, typed decoding, facet attachment, page ordering and the actual
in-memory ID-set/continuous stores. The test supplies no replacement query
evaluator, result attachment or pagination state machine.

## Complete matrix and source outcomes

The 1,944 inputs cross list/one/page, ordinary/ID-set/continuous configuration,
no/count/root/facet gate, resume/cancel/fail, offsets 0/1/99, limits 0/1/2 and both
facet restriction modes. Slice inputs affect list and first-result requests as
well as pages. A gate is reported pending only if production execution reaches it.

The original has 1,002 pending outcomes: 486 root, 408 facet and 108 count waits.
The remaining 942 finish during the first poll. There are 334 cancellations,
334 injected failures and 260 existing source errors; the other 1,016 initial
requests succeed. Successful one executes the facet stage before returning its
first entity, while its resulting entity does not retain the list's facet map.

Failure injection renames the real selected Order or Person table while its
executor request is suspended. The table is restored before retry. Original
retry has 1,632 successes and 312 `IN requires at least one value` errors in
ID-set configurations, including zero-size/empty-offset selections. These are
retained source outcomes, not successful retry or fabricated empty results.
The matrix records 7,008 ordered safe SQL diagnostic entries; that count is not
a physical statement count because failures can precede ordinary log publication.

Observations retain stage order, Pending, wake forwarding, cancellation count,
typed root JSON and complete availability, facet compact values, total count,
full errors, retry and complete stable safe SQL fields. Wall-clock start/end and
elapsed durations are excluded. No declared result, query value, error, SQL,
binding, intent or trace is normalized.

## Ownership controls

The native observer disables default PMR and deletes every actual UserContext
after allocator-aware observations leave scope. Control counters/events live in
explicit Global storage so cleanup can be inspected afterward. Executor work
owns a 32 KiB User host buffer and verifies it during cancellation before arena
blocks are reclaimed.

Additional controls delete UserContext during root/facet suspension for every
list/one/page and optimization combination, with and without explicit cancel.
Ordinary page count suspension has both deletion variants as well: 38 scenarios
in total. The stopped child must cancel once, clear its wake target and leave
zero live User bytes. This is a C++ ownership adaptation, not a claim that safe
Rust permits destroying a borrowed Context or that thread/GPU cleanup is covered.

The existing schema and dataset retain their original hashes. This does not
qualify the model's separately recorded original schema/bootstrap failure.
Broader model, Policy/privacy, store-replacement/deferred-store and full phase
integration requirements remain active under the execution qualification matrix.

## All eight generated execution bindings

A separate [application contract](../tests/generator/execution_context_binding_tests.cpp)
initializes two real generated runtimes. Every list/rows/stream/first/one/page/
count/exists call rejects the other UserContext with `ReferenceError`; SQLite
authorizers on both valid connections observe no SELECT. The same eight calls
reject the original request after its source UserContext is actually deleted.

An explicit Global copy survives, rebinds to the destination UserContext and
executes exists against its real database. This positive control must reach a
SELECT, so the negative controls cannot pass because runtime setup is missing.
Both actual UserContexts close with no remaining live User bytes. These are
runtime identity checks, distinct from the existing eight-entry compiler matrix.

## Terminal verification

| Gate | Result |
| --- | --- |
| Live original/native comparison | 1,944/1,944; immutable original corpus reproduced |
| Matching safe SQL records | 7,008 complete stable entries |
| Current relevant native integration | 9/9 |
| Imported-module checks | 3/3 |
| ASan/UBSan | 3/3; leak scanning disabled |
| Context destruction controls | 38 root/facet/count scenarios |
| Generated execution binding controls | Eight foreign and eight expired rejections, plus real positive rebind |
| Pinned formatting / canonical module consumers | 1,635 files / 786 bodies |

No runtime, provider, generator or generated artifact changed. The
[current complete execution review](parity/execution_responsibility_after_deferred_facets_5_0_6.json)
retains all 37 original bodies and three diagnostic scopes. Its native input
refresh permits only the previously qualified exact result-marker guard repair;
all other reviewed inputs are byte-identical. Identity hashes supplement the
actual source comparisons and do not establish whole-capability acceptance.

The initial C++ observer used the generated `skip` spelling directly on the
runtime facade and named a helper `request`, colliding with the core namespace.
Its full failed source/compiler output is retained in the
[preparation archive](parity/deferred_facet_page_observer_preparation_5_0_6.json.gz).
The corrected observer uses the existing equivalent `offsetOnly` runtime route
and a descriptive helper name. Generated `skip` APIs and runtime algorithms
remain unchanged.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
    teaql_deferred_facet_page_probe teaql_execution_context_binding_tests
ctest --preset native --output-on-failure \
    -R '^(deferred_facet_page_rust_parity|deferred_facet_page_context_contracts|generated_execution_binding_contracts)$'
```

The [terminal record](parity/deferred_facet_page_terminal_5_0_6.json.gz) preserves
complete paired outcomes, source identities and command output. Java/generation
reproduction, full-suite, MIPS, GPU and physical-device gates were not rerun for
these observer and contract additions.
