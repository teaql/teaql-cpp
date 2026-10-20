# Generated relation-analysis inputs and composed graph boundaries

This checkpoint follows the shared runtime work in
[relation_analysis_inputs_5_0_6.md](relation_analysis_inputs_5_0_6.md). Its remaining
requirements are explicit: complete generated facet, object-group and relation
count/detail conversions, preserve Context ownership, and qualify the actual
application calls. Matching, roll-up and field-statistic conversions remain
separate source responsibilities.

## Runtime boundaries reproduced before correction

Two native fixtures fail against the previous runtime. A NULL forward object
loses nonempty loaded facet metadata, and a second installation into the same
builder reuses view IDs and changes an earlier projection of a shared Platform.
The compiler-fixture error and both semantic failures are retained separately.

The NULL branch now installs its scoped facet metadata before retaining the
existing alias. It preserves the original NULL/NotLoaded behavior. View allocation
belongs to the builder instead of one installation call. Existing owner and child
views reserve their namespace; copies and moves preserve allocation continuity,
and exhaustion throws without wrapping. The counter owns no Context data and
adds no lifetime or global cache.

The three boundary fixtures disable the default PMR resource, inspect the actual
installed graph/facet data and delete their UserContext. Native, imported-module
and selected ASan/UBSan checks each pass these three fixtures and the unchanged
1,944-case original execution corpus. These are C++ ownership/representation
contracts, not invented Rust view-allocation APIs.

## Thin generated adapters

The producer adds public request, explicit `QuerySelection` and explicit
`SelectQuery` inputs for the original facet, object-group and relation count/
detail methods. Each has lvalue and rvalue forms. The runtime retains its checked
Context/resource validation and snapshots; the generator supplies only names
and delegates. Existing target-specific bodies remain unchanged.

The model-aware guide states which families accept these conversions, the
explicit destination-copy boundary and the facet child's required comment.
Two previous C++ target-typing refusals for unrelated facet/count children are
replaced by source-compatible compiler acceptance tests. Purposed children,
invalid scalar inputs and extra Context arguments remain compiler refusals.
Independent original code compiles these general request/selection/core inputs.

The SDK updater permits only six exact delegates per method and generated Assist.
It validates each delegate's parameters, runtime call and return category, removes
only those additions, formats the remainder and requires every old byte/file to
match. Controlled reproduction qualifies **1,824 exact delegates**, updating **22**
query headers and **30** Assist guides. All **24 workspaces / 288 files** and
**44 SDKs** reproduce strictly. No existing function body is changed.

Final gates pass native **9/9**, ASan/UBSan **6/6** with leak scanning disabled,
imported modules **5/5**, compiler **3 positive / 3 negative** and Java **60/60**.
The generated observer matches all **1,944** original outcomes, including **360**
original missing-child-comment failures. Canonical checks cover **673** bodies
and **1,485** exports; formatting covers **1,457** C++ files. These are scoped
qualification gates and do not establish full capability or phase acceptance.

The independent generated-API observer uses the unchanged 1,944-case original
corpus. The original kernel observer and its immutable qualification remain
separate. This work does not close all G06/G08/G12/G13 or P4/P5/P7 requirements.

[Final terminal evidence](parity/relation_analysis_adapters_terminal_5_0_6.json),
[current source review](parity/relation_analysis_adapters_source_review_5_0_6.json)
and [controlled SDK proof](parity/relation_analysis_adapters_sdk_change_5_0_6.json)
are separate from the previous immutable runtime checkpoint. All build/link/test
processes are terminal and successful. No MIPS, physical-device or full-suite
claim is made.
