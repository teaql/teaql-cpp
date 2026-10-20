# Generated private-field queries and real diagnostic sinks

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint repairs the reproduced enhanced-query intent-provenance gap.
Complete G14/G17 and P5/P7 acceptance remain separate requirements.

## Reproduced production mismatch

The original generated inheritance graph declares School.name as masked and
inherits that policy through Academy and ResearchAcademy. Enrollment has a
forward School relation; advisor names and campus codes have their own declared
plain policies. Existing SQLite tables are extended only with a synthetic public
`PRIVATE-SCHOOL-CANARY` test value. Original models and generated source stay
unchanged; no production secret is used.

The first actual comparison matches 1,648 of 1,728 observations. All **80**
differences are in retained logs for typed list execution. Queries and business
results already agree. C++ leaves free-text comment/purpose values visible in
cases where the original generated executor's conservative diagnostic fallback
scrubs them, including nested plain bindings. This does not mean those bindings
become masked SQL parameters: parameter policy and intent ancestry are separate.
The [initial outputs](parity/dynamic_json_privacy_initial_5_0_6.json.gz) and
[initial runtime bodies](parity/dynamic_json_privacy_initial_runtime_bodies_5_0_6.json.gz)
retain the failure without rewriting expected values.

## Shared runtime correction

The original enhanced relation-aggregate entry always creates a query scope
before fetching typed entities. It uses the diagnostic source when provided,
otherwise the actual request, and includes separately supplied aggregate queries
as diagnostic children. The generated executor retains its default conservative
query_log_intent behavior while forwarding SQL execution to the inner provider.

C++ already has the correct generated executor and redaction traversal. Its
ordinary enhanced entry only constructs a full diagnostic scope when an explicit
diagnostic source exists; the later fallback creates an empty scope. The new
shared execute_scoped_purposed_query entry captures the complete source and
separate aggregate ancestry before the existing enhanced operation executes.
Executable relation selections and SQL remain unchanged. Generated wrappers and
metadata need no patch or regeneration.

The scope is a registered Context-owned operation. Queries, service bindings,
source snapshots and aggregate storage use the same allocator; the existing
operation forwards wake/cancellation and preserves validated intent before I/O.
The [source review](parity/dynamic_json_privacy_source_review_5_0_6.json) retains
the original enhanced entry, scope body and conservative executor fallback with
source hashes. This repairs runtime responsibility rather than duplicating
privacy logic in generated models.

## Actual execution and safe-log matrix

All **1,728/1,728** original/native outcomes now agree, all successful. Four models
cross full/minimal projection, plain/readonly/facet shapes, rows/list/count/exists,
three self/nested fields per model, string/string-list operands and logging on/off.
SQLite determines the results. Facets execute only at their supported list
boundary; this matrix does not claim new persistent facet hydration.

Complete query Debug, selected result columns or typed IDs/version/readonly
values, scalar results and full timeless safe log fields are compared. Safe log
observations include SQL/parameters, expanded/pretty SQL, intent, trace,
SqlLogContext policies/masks/mode/omission and result summaries/counts. Wall-clock
timestamps and elapsed duration are deliberately outside this observation;
those are not stable cross-process values.

All **936** safe SQL records match: **480** have non-replayable masked rendering,
**336** scrubbed comments and **288** suppress unsafe SQL rendering entirely.
Some bindings to explicitly plain fields remain visible under their own policy;
using the same synthetic token does not make every field private. The 80 failing
list cases now preserve the original distinction between free-text redaction
and SQL binding policy. Disabled logs remain empty while business execution and
intent validation retain their responsibilities.

## Actual file sinks

Six fresh-process cases run against each real runtime, for **12** original/native
file-sink cases. They use one masked School.name LIKE request and compare actual
business success plus invariant sink behavior:

- Unset acknowledgement and the invalid value `true` write only safe output.
- The exact acknowledgement with a configured debug endpoint permits private
  business bindings only in that debug file.
- An exact acknowledgement without a debug endpoint creates no debug output.
- Endpoint `off` suppresses ordinary and debug output.
- Disabled query logs suppress both outputs and leave Context log storage empty.

Ordinary files and retained Context logs never contain the masked canary. Safe
files include masked SQL and `MASKED; NOT REPLAYABLE`; authorized debug output
includes `DEBUG PLAINTEXT; EXPLICIT OPT-IN`. Raw files, stdout and stderr are
retained in [repaired sink evidence](parity/dynamic_json_privacy_sinks_repaired_5_0_6.json.gz).
Timestamped file text is retained without pretending it is byte-identical between
processes. Initial sink success remains a separate pre-repair record.
These cases cover private business data, not credentials, mutation-audit sinks,
64 KiB truncation or every log formatter/environment setting.

## Ownership, reproduction and remaining requirements

The C++ observer disables default PMR, explicitly deletes each UserContext and
checks that Global live User bytes return to zero. All generated domain and
historical corpus files remain unchanged. Observer registry versions/checksums
match the locked original runtime. No production dependency, global cache or
additional ownership scope is introduced.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_dynamic_json_privacy_probe
python3 scripts/verify_dynamic_json_privacy_5_0_6.py
ctest --preset native --output-on-failure -R '^generated_dynamic_json_privacy_'
```

The module observer is `teaql_module_generated_dynamic_json_privacy_probe`; its
parity and sink checks use the same names with the `module_` prefix. The sanitizer
profile uses native names in its separate directory, with leak scanning disabled.
Relevant freshly rebuilt native integration passes **14/14**, including derived
diagnostics, persistent/combined facets, ordinary one/stream/policy, asynchronous
facet cleanup, Context controls and the generated graph example. The new imported
checks pass **2/2**, and ASan/UBSan passes **2/2**, with leak scanning disabled.
Format/check covers **1,446** files; **662** canonical consumer bodies and **1,485**
module exports verify.

See the [terminal record](parity/dynamic_json_privacy_5_0_6_terminal.json) for
completed rebuilds and relevant integration/module/lifetime gates.

Broader dynamic storage, constant/candidate/Business-ID/private-field combinations,
credential and audit consumers, cache/interleaving/async work and final full
integration remain required by the [capability ledger](cpp_generator_parity_ledger.json)
and [parity plan](cpp_generator_parity_plan.md). This matrix is additional G10/G14/
G17 evidence, not a complete capability or phase acceptance. No Java/generation,
MIPS or physical-device run is inferred.
