# Clock, Context-root and inherited model composition: Rust 5.0.6

Status: 216 exact original/native cases compare 1296 field outcomes. The
demonstrated Checker condition gap is repaired. G02/G03 remain partial pending
their final responsibility and integration review.

The [composition model](../tests/generator/checker_clock_composition.xml)
combines an inherited string alias, a compatible integer definition, required
JSON, a Context-root FK and ordinary required/optional FKs. Parent and child
keep distinct types and metadata. The observer imports complete original
Tenant/Record/Note entity and Checker files unchanged.

An initial tally alias had i64 storage but generated String initialization and
conversion code. C++ rejected the storage mismatch. The
[initial prototype archive](parity/checker_clock_composition_initial_alias_failure_5_0_6.json.gz)
preserves that model, generated source and both failures. The current model uses
the previously qualified integer definition. Its full original crate still has
a [runtime assembly error](parity/checker_clock_composition_complete_source_error_5_0_6.json.gz):
module_with_behaviors_and_checkers is referenced without being generated.
The component harness does not qualify that assembly or request execution.

## Source condition gap

The initial comparison found **88/216 differences** in Context-root assignment,
validation and findings. The [initial results](parity/checker_clock_composition_initial_gaps_5_0_6.json.gz)
remain unchanged. Both parsers report the same owner attribute:
`contextRoot = true,createFunction=now,updateFunction=now`.
These comma-containing parameters remain one attribute value; the strict Boolean
conversion returns false for that string.

The original Checker template uses SKYNETObjectAdaptor. Exact Boolean tokens
true/false/on/off/yes/no become Boolean; other nonempty values become String.
StringTemplate considers the String true. C++ previously used strict Boolean
conversion and omitted the generated Context-root rule.

C++ Checker emission now uses sourceTemplateFlag for contextRoot, createFunction
and updateFunction. Exact false/off/no suppress the branch; other nonempty values
enable it. Parsing, metadata and setter-visibility decisions retain their separate
source responsibilities. The existing runtime Context-root implementation is
reused. [Actual metadata](parity/checker_clock_composition_metadata_5_0_6.json)
and Java regression verify the distinction and false/off/no clock suppression.

Inherited type definitions declare clock attributes separately, so both branches
execute for those fields. Comma-containing relation parameters do not independently
declare both attributes. Earlier audits prove the generated branches, not an
assumption that all requested attributes were parsed.

## Semantic and lifetime evidence

The 216 cases cross three statuses, three active-root states, three FKs, full/
sparse loading, present/NULL optional FK and two fixed process clocks. Six fields
compare stored getters, exact mutation values, Timestamp variants and availability.
Required JSON stores numeric time; scalar/FK conversions retain their native
values. Findings retain complete locations and all three wire naming profiles.

Native observations copy explicitly into Global. Local PMR scopes end and
UserContext is actually deleted before the copied result is consumed; no charged
User blocks remain. Default PMR is disabled. This direct Checker matrix executes
no SQL and claims no committed save.

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_checker_clock_composition_probe teaql_sample_fixed_clock
python3 scripts/verify_checker_clock_composition_5_0_6.py
```

The verifier checks frozen artifacts, reference versions/checksums, locked offline
compilation and live results, with hashes for all six imported original files.
Original failed workspaces remain unchanged. Broader Checker/Behavior, assembly,
provider, cache, governance and intent/privacy obligations remain open.

Selected native checks pass 3/3, canonical module checks 1/1 and ASan/UBSan
1/1 with leak scanning disabled. Java passes 48/48. All 23 workspaces/276 files
and 44 SDKs strictly reproduce; 584 canonical bodies and 1404 pinned-format
C++ files verify. The [terminal record](parity/checker_clock_composition_5_0_6.json)
and [source record](parity/checker_clock_composition_source_5_0_6.json) retain
results and fingerprints. No MIPS or physical-device execution was performed.
