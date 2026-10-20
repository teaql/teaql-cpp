# Matching, roll-up and field-statistic input conversions

The locked source accepts `Into<QuerySelection>` for forward/reverse matching,
roll-up and relation field-statistic methods. This checkpoint follows
[the general relation-analysis adapters](relation_analysis_adapters_5_0_6.md)
while preserving each family's different conversion boundary.

## Source responsibilities and runtime implementation

Matching uses the declared relationship target's descriptor, independently of
the incoming request's entity or executor. It embeds the selection's bare query
in its predicate and retains the complete selection in `relation_filters`.
The runtime now has an explicit expected-descriptor entry point for checked
unpurposed requests, `QuerySelection` and `SelectQuery`. Existing typed entry
points remain compatible and delegate to the same capture.

Roll-up combines matching and object grouping. Generated inputs delegate both
operations to the shared runtime using the declared target descriptor. They do
not execute a child provider or change the aggregate boundary.

Field statistics first materialize `QuerySelection::into_query()`, add the
specific aggregate and store the resulting bare query in a fresh selection.
The runtime preserves its existing execution/options boundary and diagnostic
origin. All eight source field-statistic families receive explicit selection,
core-query and cross-executor request capture without a default allocator.

Every wrapper validates Context identity. Values require the destination
resource; cross-domain retention needs an explicit copy. Both fluent value
categories are supported, and purposed children remain unavailable. The memory
fixture disables the default PMR resource, mutates inputs after capture, rejects
foreign wrappers and values, checks explicit Global copies, deletes the User
and reads retained state through a new UserContext.

## Original execution evidence

All **2,808/2,808** original/runtime outcomes agree without normalization.
Thirteen entry points cover forward/reverse positive and negative matching,
roll-up, all eight scalar statistics, full/minimal parents and children, six
child configurations, request/selection/core conversion and rows/list/page
execution. The complete corpus includes `relation_filters`, query/options,
child immutability, typed JSON/load state, errors, SQL, bindings, intent and trace.

There are **1,992 successes**, **3,424 result rows** and **4,080 safe SQL records**.
The **816 failures** preserve the original multi-column subquery and unsupported
SQLite function errors. Emitting a statistic does not add a SQLite extension or
turn an unavailable SQL function into successful execution.

The first 2,808-case corpus, which did not expose the complete filter sidecar,
remains immutable. A separate complete corpus adds that observation and retains
the same inputs; its original outputs come from the unchanged Rust methods.
The independent Rust lock retains reference registry versions/checksums. A
separate original program compiles unrelated request/selection/core inputs for
matching, roll-up and every field-statistic family.

## Generated adapters and qualification

The producer adds thin public request and explicit value conversions. Matching
descriptors are emitted from the declared relation type, and field-statistic
function/field/alias names come from the existing shared naming metadata.
Existing target-specific functions retain their bodies. Assist describes the
different metadata boundaries, allocation requirements and SQLite failures.

The controlled SDK updater validates every new body's exact runtime delegation
and value category, removes only those additions, formats the remainder and
requires every old file/body to match. An initial verifier rejected two overloads
because clang-format split a long method name from its parameter list; the
whitespace matcher was corrected, and the failure is preserved.

Final generated application/native gates pass **7/7**, selected ASan/UBSan
**3/3** with leak scanning disabled, imported modules **3/3**, compiler
**1 positive / 3 negative** and Java **61/61**. All **24 workspaces / 288 files**
and **44 SDKs** reproduce strictly. Controlled updates qualify **1,500 exact
new delegates**, with every old body/file retained after removal and formatting.
Canonical checks cover **676** bodies and **1,485** exports; formatting checks
**1,460** C++ files. All build/link/test processes are terminal and successful.
Complete G06/G08/G12/G13 and P4/P5/P7 requirements remain active. These input
conversions do not establish full generator/runtime parity.

Reproduce the original and generated application comparison after building
`teaql_generated_matching_statistics_adapters_probe`:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_matching_statistics_inputs_5_0_6.py
```

[Terminal evidence](parity/matching_statistics_terminal_5_0_6.json),
[source review](parity/matching_statistics_inputs_source_review_5_0_6.json) and
[controlled SDK proof](parity/matching_statistics_sdk_change_5_0_6.json) retain
the scoped qualification. No full-suite, MIPS or physical-device result is claimed.
