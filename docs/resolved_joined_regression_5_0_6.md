# Joined resolved-service regression and G09 reopening

The first current native union rebuilt all 16 selected targets and ran 31 tests.
Its [retained failed receipt](parity/resolved_native_closure_5_0_6.json.gz) records
29 passes and two failures: query entry/cursor matched 7,744/8,000 and aggregate
membership matched 10,304/10,560. Both failures include 256 ordinary-row shortcut
observations where inline aggregates incorrectly entered enhancement. The
preceding focused SmartList diagnostic-scope correction passed its eight controls.

The current `EntityRowsOperation` merged inline aggregates unconditionally after
preparation, contradicting the locked original row-entry guards and the earlier
[documented contract](query_entry_cursor_5_0_6.md). The row entry must select
whether enhancement applies before moving inline declarations. An ordinary direct
row query retains those declarations in its provider request and does not publish
enhancement cursor observations. Explicit aggregate, prepared, relation, child,
object-group and continuous-page entries retain their enhancement path.

The candidate now guards normalization with exactly those enhancement conditions.
The existing merge, provider call, membership snapshot and cursor publication
mechanics remain unchanged. This repair restores the documented source distinction;
it does not redefine expectations to accept additional C++ work.

Historical reopening: G09 was **partial** pending requalification. The
[reopening record](parity/resolved_joined_g09_reopened_5_0_6.json.gz) retains its
previous capability entry and ledger hash. Previous qualified EX01-EX14 evidence
is historical; current failures cannot be hidden behind it. Requalification must
retain the original acceptance contract, correlate unchanged evidence explicitly,
and complete the affected current native/module and lifecycle gates. The new
31-test union alone is not a substitute for all capability exit criteria.

The [repaired native union](parity/resolved_native_closure_v2_5_0_6.json.gz)
passes 31/31, including the full 8,000 query/cursor and 10,560 membership corpora
and existing Context lifecycle controls. The [module union](parity/resolved_modules_closure_5_0_6.json.gz)
also passes all 31 equivalent commands, with 1,130 canonical consumers and
unchanged inputs. Whole resolved-source acceptance and restored G09 acceptance
are not claimed yet. The original G09 native37/module10 regression selection
is the next gate, with the complete EX01-EX14 acceptance review kept separate. Active source totals remain
112 accepted / 10 pending / 2 excluded_external; capabilities are 10 qualified /
7 partial. P0-P7 remains incomplete.

Current verdict: [G09 requalification](resolved_g09_requalification_5_0_6.md)
restores the original capability after complete current gates and evidence
reconciliation. Capabilities return to 11 qualified / 6 partial. The failure
and reopening records above remain immutable; source totals do not change.

## Joined regression after the flat-graph repair

The [new native receipt](parity/resolved_native_closure_flat_graph_5_0_6.json.gz)
and [matching imported receipt](parity/resolved_modules_closure_flat_graph_5_0_6.json.gz)
each pass all **37** selected checks. The original 31 remain intact; the union now
also includes private aggregate scope, validated ID-set intent, custom/concrete
cache execution, flat predecode and both generated analysis input corpora. These
checks qualify the interaction of the installed graph capability with previously
correct query/cursor, membership, schema, dynamic loading and mutation paths.
The two gates hold 1,403 input fingerprints unchanged. Core Entity defaults and
the generated relation fixture are now included in the frozen inputs, so the
representation capability cannot change unnoticed during execution.

The module controller's terminal message still printed its historical hardcoded
31/31 count. The retained receipt enumerates all 37 required and successful test
names, with no omissions. The display is corrected to use the selected count;
no command, corpus or acceptance assertion changes. Canonical module validation
passes 1,134 bodies. This is a scoped joined regression, not a full native suite.

The separate [typed dynamic cleanup continuation](resolved_dynamic_loaded_5_0_6.md#typed-caller-cancellation-continuation)
adds eight C++ lifecycle scenarios across four entries and both cancellation
routes. The complete existing dynamic-query executable passes native, imported
and focused ASan/UBSan. Runtime and generated bodies remain unchanged in this
continuation; the prior flat-graph repair remains installed. Captured lifecycle
build logs (1,687 bytes) are compressed in its receipt and removed; stable preset
outputs remain in place. No disposable probe build tree is introduced.

Complete resolved-source review and P0-P7 remain open. Source counts remain
112 accepted / 10 pending / 2 excluded_external; capability counts remain
11 qualified / 6 partial. No new physical-device, MIPS or release claim is made.
