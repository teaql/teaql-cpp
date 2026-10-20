# Generated request result markers: Rust 5.0.6

This checkpoint follows the [runtime-first repair](query_result_marker_5_0_6.md).
Generated requests now preserve the original distinction between an ordinary
request's unconstrained result marker and the public Entity-bound execution
facade. It addresses this named G06/G09 discrepancy; complete capability and
P0–P7 acceptance remain separate.

The original reference is generator
`45e1572c4956ca963410d8ea436ead3c8857087a` and Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The complete original guard/body
review and actual nine Rust compiler refusals are retained by the earlier
checkpoint. The locked sources and original generator checkout are unchanged.

## Generated contract

`Request<Purposed, Result>`, `returnType<T>()` and child-result conversions no
longer impose Entity during construction. They remain thin wrappers over the
qualified shared runtime. Matching, enhancement and relation selections accept
these ordinary requests without constructing their marker type.

All eight execution entries and `newEntity` require Entity, including compact
rows/count/exists. The three existing native execution aliases retain the same
guard. Purpose remains mandatory; execution still receives the bound
`user_context` and validates nonblank intent through the existing runtime.

For example, after obtaining the generated factory and setting up its runtime:

```cpp
auto pending = Q.schools().returnType<std::uint64_t>().selectName();
auto request = std::move(pending)
                   .returnType<School>()
                   .comment("Count the selected schools")
                   .purpose("Verify the restored entity result");
auto count = require_value(request.executeForCount(user_context).wait());
```

Calling `executeForCount` on the non-Entity marker itself is a compiler error.
Changing the result marker does not change the source entity, load fields,
execute SQL or alter allocation ownership. Unexecuted requests still belong to
their Context. Existing native fluent operations after purpose remain available;
this checkpoint does not claim identical Rust/C++ method sets at every stage.

## Controlled generation and source evidence

The [before snapshot](parity/query_result_marker_sdks_before_5_0_6.json.gz)
retains all 77 complete SDKs. Java generation, pinned formatting and controlled
refresh scripts reproduce their new state. The
[SDK proof](parity/query_result_marker_sdk_update_5_0_6.json) admits only:

- unconstrained Request/result/friend/child-result template parameters;
- Entity constraints on execution/creation signatures;
- one exact model-specific Assist paragraph explaining this boundary.

Across 179 request types, 2,148 execution/creation guards are explicit. Every
pre-existing function body and every other generated code/metadata file remains
unchanged. Producer fingerprints are refreshed through generation, rather than
editing generated files. The separate historical E-entry, scalar diagnostic,
ledger-composition and relationship proofs strip only this new bounded change
when replaying their own earlier checkpoint; their old evidence is preserved.

The application observer now calls generated `returnType` and generated child
selection methods directly. Its existing 144-case original corpus compares all
eight complete query/options/selection fields, including arbitrary markers on
child requests and transient Unicode/NUL text. This is construction/inspection
evidence. The ownership test separately restores Entity and performs actual
SQLite execution, and existing source/result-model SQLite regressions retain
their separate outcome matrices.

The former `return_type_not_entity` compiler expectation is preserved in the
[producer/compiler before snapshot](parity/query_result_marker_producer_before_5_0_6.json.gz).
It is replaced by positive arbitrary-marker construction/composition/restoration
and negative execution/creation checks, including all three native aliases.
Neither successful rendering nor the number of declarations proves behavior.

## Verification

| Completed gate | Result |
| --- | --- |
| Generated facade / actual original state comparison | 144/144, eight complete fields each |
| Relevant native regressions and examples | 8/8 |
| Imported module checks | 3/3 |
| Targeted ASan/UBSan | 2/2; leak scanning disabled |
| Generated API compiler contracts | 2 positive / 14 negative |
| Java generator and field/JSON metadata checks | 70/70 |
| Strict ordinary generation | 24 workspaces / 288 files |
| Strict contract generation | 50 cases, including retained source-invalid fixtures |
| Complete identity/root fixture reproduction | Two identity pairs and the original/native root pair |
| Controlled SDK update | 77 SDKs / 179 request types; existing bodies unchanged |
| Historical repair proofs | E entries, scalar diagnostics, ledger composition and relationship identities pass |
| Pinned formatting / canonical module consumers | 1,630 handwritten files / 781 bodies |

All builds and compiler processes completed before their produced artifacts
were consumed. The independent runtime-first evidence remains historical;
these gates exercise the regenerated facades and their explicit constraints.

The [terminal evidence](parity/query_result_marker_generated_terminal_5_0_6.json.gz)
records final native/module/lifetime/compiler/Java gates, source identities,
complete paired observations and strict regeneration. The source review for
existing field interfaces is refreshed separately in
[the current record](parity/predicate_projection_source_review_after_result_marker_5_0_6.json)
without replacing previous source/body reviews.

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_query_result_marker_source_5_0_6.py --check-original-compiler
python3 scripts/verify_query_result_marker_sdk_update_5_0_6.py
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
python3 scripts/generate_relation_identity_contracts_5_0_6.py --check
python3 scripts/generate_expression_root_contracts_5_0_6.py --check
cmake --build --preset native --target \
    teaql_query_result_marker_probe teaql_generated_request_state_tests
ctest --preset native --output-on-failure \
    -R '^(query_result_marker_runtime_rust_parity|generated_request_state_context_contracts)$'
```

No runtime dependency, ownership domain, provider, execution algorithm or
implicit loading path is added. Full-suite, MIPS, external-provider, GPU and
physical-device acceptance are not claimed by these focused checks.
