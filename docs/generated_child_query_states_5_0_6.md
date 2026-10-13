# Embedded generated query state against the locked producer

Status: G17/G06/G08/G09 scoped call-state and callable-declaration contracts pass; full capabilities remain partial. Runtime:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; producer:
`45e1572c4956ca963410d8ea436ead3c8857087a`. Complete capability/P7 acceptance
remains open.

## Child selection conversion

The original generated requests implement `From<Request<R>> for QuerySelection`.
Their `PurposedQuery<Request<R>>` wrappers do not implement that conversion.
Actual original compilation accepts a plain Profile child in
`PlatformRequest::select_profile_with` and rejects a purposed child with E0277,
naming the missing `From<PurposedQuery<ProfileRequest>>` implementation.

C++ previously accepted either child state through unconstrained bool templates.
The runtime now restricts QuerySelection capture and typed embedding entry points
to unpurposed children. Relation selection, enhancement, matching, object groups,
facets and relation aggregates share this state boundary. Existing Context/source
binding checks, explicit destination copying and allocator propagation remain.

Generated adapters retain both parent states and ref-qualified chaining. Their
child bool templates require `!P`; generic enhancement requires a valid child
selection conversion. They delegate combination to the runtime instead of
duplicating its mechanics. Assist explains that child filters/comments belong
to the ordinary child request and root execution owns its purpose.

## Parent modifier adaptation

The agreed C++ API allows modifiers before or after the parent's purpose
transition. For these selectors, the original Rust PurposedQuery has no direct
forwarding method. The compiler comparison therefore explicitly takes its
public `inner` request, combines the child and restores the same parent purpose.
It does not claim identical Rust/C++ spelling or direct Rust wrapper method
availability. This parent adaptation does not make a purposed child convertible.

## Compiler matrix

`scripts/verify_generated_child_query_states.py` compiles actual unchanged original
generated requests and the current C++ facade. Eight operation families cover
selection, enhancement, facets, matching, grouping, roll-up, reverse count and
reverse stats. Each varies both parent and child state. Valid cases must compile;
invalid original cases must name E0277/QuerySelection/PurposedQuery, and C++
rejection must come from constraints. Unrelated compiler failures fail the check.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_child_query_states.py \
    --build-dir build_native_current \
    --rust-target-dir /tmp/teaql_cpp_field_layout_rust_506
```

The final matrix matches all **32** actual original/C++ call-state contracts (16 valid and 16 invalid). It first rebuilds the locked original dependency and additionally passes **32** callable-declaration constraints, each checking both lvalue and rvalue parents. Rejecting only inside a method body cannot masquerade as a constrained API.

The generator regression passes **29/29**. Targeted native query binding,
generated API, relation, inherited-query, facet and relation-analysis tests pass
**6/6**; the broader selected native/source regression passes **9/9**, including original facet, relation-analysis and inherited-query corpora. Module Context-root and sample-field import contracts pass **2/2**. The complete native build passes. Final Assist refresh and reproduction pass for all **18 workspaces / 216 files**.

This is an API-state check, not full SQL execution/privacy qualification for
each family. General matching/conversion surfaces, all other model/result-type
combinations and broader governed/protected-I/O contracts remain open. No
sanitizer/MIPS acceptance is inferred from compile-time constraints.

The subsequent [ID membership checkpoint](generated_matching_any_5_0_6.md) adds
matchingAnyOf and matchAnyOf. Its separately selected run passes eight additional
original/C++ state cases and eight callable constraints with both reference
categories. The verifier now includes these families in its default selection;
the earlier 32-case checkpoint remains historical evidence rather than a claim
that the expanded default matrix was run as one command.
