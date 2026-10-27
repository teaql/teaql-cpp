# Generated expression entry identity

Current canonical-root follow-up: [EID01 requalification](generated_expression_identity_5_0_6.md) separates model identity from descriptor overrides and native entry spelling. Runtime-first capture and thin metadata now match960/960 original/native outcomes; native5/module3/scoped SAN1/compiler4-positive6-negative/Java88 and80-SDK restricted reproduction pass. G11 is qualified again; G13/G14/G17 and P0-P7 remain open. Earlier checkpoint APIs and evidence below retain their original scope.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint extends G11/G14 identity evidence. Complete G14 and P0–P7
acceptance remain open.

## Reproduced mismatch

An entity named `class` has canonical identity `Class`, and its C++ expression
factory is `E::modelClass(value)`. Before this repair, runtime diagnostics derive
the entry from the canonical type and display `E::class(...)`. The same issue
affects `operator`, `template` and `namespace`.

The complete original and native generated libraries are retained with source
provenance. Their actual SQLite execution initially agrees in 320 of 560 cases.
All 240 differences are confined to `original_expr_with_broken_point`: 60 for
each keyword entity. Canonical identity, paths, other diagnostic fields, values,
query state and SQL logs already agree. The ordinary `plain_record` entity is
an independent control. See the [full initial comparison](parity/expression_entry_initial_5_0_6.json.gz).

## Runtime first, then generated metadata

The runtime now accepts an explicit static entry name independently of the
canonical entity description. It reuses the existing `StaticString` abstraction;
generated names are string literals, and runtime User-backed buffers cannot be
passed through this API. No new naming heuristic, metadata registry, ownership
domain, singleton or dependency is introduced.

Expression root provenance keeps that name across map/traversal branches,
scalar destination copies and diagnostic/error destination copies. Context
checks and entity lifetime checks remain active. The original factory signatures
remain available; separate overloads accept metadata, including through the
same one-argument entity and two-argument scalar factory pointer types.

The generated adapter supplies its actual factory name:

```cpp
static ClassExpression modelClass(const Class& value) {
    return ClassExpression(entity_expression(value, StaticString{"modelClass"}));
}
```

Business code still calls `E::modelClass(value)` with one argument. Generated
adapters contain no diagnostic rendering or lifetime algorithm. Canonical root
`Class(id=10)`, field paths and selection suggestions remain unchanged; the
native expression display now names its actual C++ factory.

Factories without explicit metadata retain the earlier generic rendering rule.
The new overload does not infer a model-specific API from an arbitrary type name.
No extra JSON diagnostic field is added.

## Ownership and cost

The original canonical root description remains a Context-owned snapshot. The
entry name refers to generated program metadata; it does not retain an entity,
Context or runtime string buffer. Wrapper expiry and explicit destination-copy
rules remain unchanged.

At the native kernel checkpoint, `String` is 40 bytes, `StaticString` is 16 bytes,
and the root-provenance structure grows from 40 to 56 bytes. The new field also
adds a `StaticString` value to an allocated diagnostic. This adds no arena object,
allocation ID or character allocation for the entry name. The cost applies per
allocated provenance/diagnostic, not per complete expression chain. Existing
allocation budgets continue to account for arena storage.

The kernel contract checks immutable canonical root capture, skipped missing
callbacks, mapped/nested missing states, explicit scalar and error retention,
actual source UserContext deletion and expired access rejection. It disables
default PMR allocation. Application compiler controls additionally reject a
runtime string, an automatic character buffer, temporary entities, an extra
Context and the keyword spelling `E::class`.

## Actual source comparison

The unchanged frozen Rust SDK executes through its actual generated runtime and
SQLite provider. The observer's registry versions/checksums must match the
reference dependency graph. Its immutable corpus crosses five entities, seven
projection configurations, direct/nested reads, active/deleted IDs, deleted-row
inclusion and `eval`/`unwrap`.

The 560 original outcomes contain 300 missing-load diagnostics, 120 scalar values
and 140 absent rows, with 680 ordered SQL records. Complete query Debug, presence,
values, error text and the declared SQL/binding/intent/trace fields are compared.

The existing C++ diagnostic spelling/English adapter remains in use. One explicit
presentation extension selects the actual generated C++ entry from the complete
SDK factory declarations. It changes only the expression's leading E entry;
canonical root identity, paths, fixes and all business/SQL fields are preserved.
This is a declared C++ naming adaptation rather than Rust diagnostic text parity.

## Exact generation and previous gates

The controlled update checks 77 complete SDKs against their preserved bytes.
Exactly 179 existing E factory calls receive a static literal equal to that
factory's existing method name. Every other generated code/Assist byte, including model metadata,
query/model implementations, field helpers and Assist, remains unchanged.
Original Rust libraries and pre-repair C++ output are retained.

The earlier relationship, ledger-composition and scalar keyword proofs remove
only these independently qualified later arguments before replaying their own
changes. Their original snapshots and reports are preserved. The source review
also confirms unchanged bodies for all 63 locked field methods, the query
constructor and all nine previously reviewed generator helpers.

Initial implementation checks exposed two additional issues: map/traversal
reconstruction omitted the entry metadata, and adding a default parameter
changed exact function-pointer signatures. Both are covered by explicit
regressions. The full initial kernel/signature records are retained. A separate
proof-helper failure involved pinned formatting wrapping a long factory call;
the checker now accepts whitespace while still requiring the exact argument.

## Verified checks

| Check | Completed result |
| --- | --- |
| Actual original/native, imported-module and instrumented comparison | 560/560 per frontend |
| Final native regression | 7/7 |
| Imported-module regression | 2/2 |
| Targeted ASan/UBSan, leak scanning disabled | 2/2 |
| Application compiler controls | 2 positive, including original function-pointer types; 5 negative |
| Java producer checks | 69 passed |
| Exact generated-body proof | 77 complete SDKs; 179 static factory arguments |
| Strict reproduction | 24 normal workspaces, 50 contract fixtures, two previous keyword pairs, the new root pair and alias entry |
| Historical proof replay | Scalar keyword, ledger composition and relationship identity proofs pass |
| Pinned formatting and check | 1,628 C++ inputs |
| Canonical imported bodies / catalog exports | 779 bodies / 1,495 exports |

Earlier native selections also passed 7/7 and 4/4, including related scalar and
configured-model E comparisons. Final formatting and canonical consumer counts
are retained in the terminal record; historical evidence is not relabeled.

## Reproduction

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_expression_entry_sdk_update_5_0_6.py
python3 scripts/generate_expression_root_contracts_5_0_6.py --check
cmake --build --preset native --target \
  teaql_expression_entry_tests teaql_expression_root_probe
ctest --preset native --output-on-failure \
  -R '^(expression_entry_contracts|expression_root_rust_parity)$'
```

Imported-module consumers use the same canonical test/observer bodies. Selected
ASan/UBSan checks cover the changed provenance state, with leak scanning disabled.
Final commands, source hashes and completed results are recorded in
[terminal evidence](parity/expression_entry_terminal_5_0_6.json.gz).
No full-suite, MIPS, physical-router, external-service or complete generator-parity
result is implied.
