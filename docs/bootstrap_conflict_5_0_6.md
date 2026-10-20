# Bootstrap optimistic conflicts with a second SQLite connection

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: this deterministic interleaving matrix passes; complete G05/P3 remains partial.

## Real conflict injection at the public plan boundary

The complete unchanged Numeric generated domain runs its original public module,
SQLite provider, ID service and `ensure_schema` APIs. Native uses its complete
generated counterpart and the existing runtime/service/executor/provider layers.
Neither observer replaces bootstrap, graph planning, optimistic version checks,
transaction handling or the generated retry classifier.

Two distinct SQLite connections open one shared in-memory database. Authentic
bootstrap creates the schema and data, then fixture SQL makes one Priority's rank
drift from its seed. An application-owned Mutation Policy observes the complete
immutable update plan. At that boundary, before the runtime begins its write
transaction, the second connection commits a real version increment. The policy
allows the original plan; ordinary persistence must detect its stale version.

This is a controlled interleaving of two real connections, not simultaneous-thread
execution, concurrent access to one UserContext or a timing-dependent stress test.
The same callback position is established by original `GraphSaverFor::save_graph_dyn`:
plan, policy review, transaction begin, execution. The rival can either leave the
scalar drift intact or repair it while incrementing the version.

Policy instances belong to UserContext and borrow the Global-owned rival resource
and observation storage. Actual User deletion happens before the retained database
is observed again. Native disables default PMR allocation. No shared owner extends
the User lifetime or cancels another Context's work.

## Exact matrix and outcomes

The **768** cases cross four module modes, target Priority 1/22, initial version
1/7, requested rival writes 0/1/2/4/5/6, one/two initialization rounds, log switches,
and version-only versus scalar-repair rivalry. They contain **1,152** rounds,
**1,824** immutable plan reviews, **1,440** actual rival commits and **6,568** SQL
diagnostic entries.

All complete results agree without value/error/ID normalization: physical rows,
versions, ID floors, plans and original versions, rival before/after state,
remaining conflict budget, per-round review counts, exact errors, SQL/parameter/
intent diagnostics, autocommit and data after User deletion. Fixed trace/user
configuration makes execution IDs reproducible while keeping them in the output.
Operational clock/timing fields are outside this observer's output.

There are **1,024** successful rounds and **128** exact exhausted optimistic
errors, split equally between the two target identities. Autocommit is restored
in every observed round. For target 1 starting at version 1:

| Rival writes requested | Rival action | First round | Second round |
| --- | --- | --- | --- |
| 1 | Increment version | Two plan reviews; success at version 3 | No-op at version 3 |
| 4 | Increment version | Five reviews; success at version 6 | No-op at version 6 |
| 5 | Increment version | Five reviews; optimistic error, rival version 6 retained | One review; success at version 7 |
| 6 | Increment version | Five reviews; optimistic error, one rivalry remaining | Two reviews; success at version 8 |
| 6 | Repair rank and increment version | One failed stale-plan attempt, then successful no-op at version 2 | No-op; five rivalry actions remain unused |

These outcomes confirm the original five-attempt boundary rather than assuming
every failed initialization rolls back a competing writer's committed work.
The retry rereads current state and can finish without writing when the rival
already supplied the modeled value. Exhaustion is returned as an error, and a
later initialization may recover. No entire-bootstrap atomicity is invented.

## Initial fixture correction and retained evidence

The initial comparison recorded 768 differences because complete Policy Debug
included default process/time-derived execution IDs. A focused comparison showed
rows, retry outcomes and SQL already agreeing. Both observers now set the same
trace and diagnostic user label before execution. No output key is removed or
rewritten. The [complete initial differences](parity/bootstrap_conflict_5_0_6_initial_gaps.json.gz)
are retained with lossless gzip compression.

The observer includes an existing physical SQLite serializer; its unused renamed
baseline main produces a compiler return warning. It is not executed, and the
active observer has its own normal main/exception boundary. No production
function or generated file was modified for this fixture.

## Verification and remaining scope

Formal native and module tests run the same retained source corpus without Java
or temporary generated headers. Live original reproduction remains a separate
command. Provenance checks cover the complete original and native generated
domains before comparison. Native and module selections each pass **1/1**, all
**322** canonical consumer/fixture bodies verify, and pinned format/check covers
**1,213** C++ files. The [terminal record](parity/bootstrap_conflict_5_0_6.json)
records successful builds/tests and current source/corpus hashes.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_constant_variants/Cargo.toml --bin bootstrap_conflict
cmake --build --preset native --target teaql_bootstrap_conflict_probe
python3 tests/parity/compare_bootstrap_conflict_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/bootstrap_conflict \
  --cpp build_native_current/teaql_bootstrap_conflict_probe
ctest --preset native --output-on-failure -R '^bootstrap_conflict_rust_parity$'
```

The module target is `teaql_module_bootstrap_conflict_probe`; its test is
`module_bootstrap_conflict_rust_parity`. Simultaneous-process scheduling, broader
root/dependency topologies, custom Checker/behavior/resource overrides and full
phase/capability integration remain separate. Default generation guards remain.
No sanitizer, MIPS or physical-device result is claimed.
