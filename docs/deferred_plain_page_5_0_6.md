# Ordinary deferred count-first pages

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint tests ordinary page execution through deferred actual queries.
ID-set store waits retain their separate checkpoint.

## Source and native execution boundaries

The complete unchanged Rust domain's `execute_for_page` runs through its public
`TeaqlRuntime` SPI. A test adapter selects a custom executor which suspends only
before delegating a count or row request to the original domain SQLite executor.
Count/list repository operations and typed decoding remain original production
methods. All other SPI entries delegate the existing runtime rather than providing
local query or persistence implementations.

C++ uses the public shared `EntityQuery<Order, Executor>` facade over the actual
generated Order model. Its custom executor selects the same deferred boundary
and delegates to the actual generated domain executor. The executor is resolved
through UserContext; no second service/provider argument is supplied to execution.
The ordinary page state machine, count conversion, list service and SQLite
provider remain production algorithms. Both observers use the same explicit
existing schema/data and preserve the documented original bootstrap failure.

## Declared scenarios

Inputs cross three gate positions (none, count, rows), resume/cancel/fail actions,
offsets 0/1/99 and limits 0/1/2/10. Count must precede the typed row query. A zero
limit can avoid a row query and therefore need not reach a requested row gate.
Recorded events preserve that distinction rather than manufacturing Pending.

Actual failure is injected by renaming the real Order table while the selected
query is pending; neither observer fabricates query results or errors. The table
is restored before a new page request verifies executor reuse. Cancellation drops
the original future or explicitly cancels the native operation, then executes the
same page again. Observations include stage ordering, Pending, propagated wake,
rows/versions/totals, exact errors, cancellation count and complete retry result.

The original application executor type is fixed by the generated domain. The
Rust SPI adapter and C++ compile-time executor parameter provide the supported
extension boundary for the test. They do not alter generated declarations or
invent an executor parameter in application execution APIs.

## Remaining scope

Continuous cursor replacement/interleaving, broader grouped/facet/inherited/
constant/dynamic model configurations, protected I/O and complete integration
remain separate responsibilities. This does not close all G09/G13/P0-P7 work.

## Ownership and observed outcomes

The full 108-case matrix agrees without normalization: 60 initial successes,
24 controlled cancellations and 24 actual missing-table errors. Restored tables
and released requests allow every retry to execute. Native controls use explicit
Global storage so counters can be inspected after User destruction; request,
result and operation payloads remain User-owned. The default PMR resource is
disabled. Four extra native scenarios actually delete UserContext during count
or row waits, with/without prior explicit cancellation, and require exactly one
stop with no remaining callback. These are C++ ownership adaptations, not Rust
Context deletion while its future still holds a safe borrow.

## Terminal verification

The 108 actual original/native cases reproduce against live original Rust; all
four native ownership cases pass. Native, targeted ASan/UBSan and imported module
selections each pass **2/2**, after successful builds; sanitizer leak scanning
is disabled. Canonical consumer verification covers 160 bodies, with 1,479
catalog exports and seven acyclic module dependencies. Pinned format/check covers
1,021 handwritten C++ inputs. See the
[terminal record](parity/deferred_plain_page_5_0_6.json). No runtime function,
generator body or generated artifact changed. Java/generation reproduction and
MIPS/physical-device gates were not repeated.
