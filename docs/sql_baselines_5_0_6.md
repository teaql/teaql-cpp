# SQL compiler and transport baseline qualification

This checkpoint advances P7 integration and the SQL prerequisites of G06/G08/G09,
G13 and G14. Complete capabilities and P0-P7 phases remain open.

## Source and unchanged historical inputs

The runtime remains locked to Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Five unchanged historical development
drivers call the original public SQL, binding, transport and projection APIs.
The independent observer manifest is
`tests/parity/rust_sql_baselines_5_0_6/Cargo.toml`; its dependencies and lock are
development fixtures, not C++ production dependencies.

| Historical corpus | Inputs | Changed source outcomes |
| --- | ---: | ---: |
| Identifier/storage helpers | 262 | 0 |
| Direct IN compiler/default failures | 216 | 0 |
| Noncopyable projection payload | 8 | 0 |
| Transport defaults | 344 | 0 |
| Schema identifiers/indexes | 1,305 | 0 |
| Mutation compilation | 2,505 | 0 |
| Mutation provider hook order | 48 | 0 |
| Query/expression dialect compilation | 2,206 | 12 |
| Total | 6,894 | 12 |

Historical driver bodies, shared Rust parsing helpers and historical corpus
bytes remain unchanged. Each active corpus records its input-file hash, driver
hash and observer manifest/lock/helper hashes. All outputs are compared exactly,
including SQL, values, parameter policies, errors and hook order. No output
normalization or input filtering is used.

Numbered-placeholder fixture dialects exercise the existing generic compiler
contract; they do not add or verify another database provider. Executed database
regressions below use SQLite only.

## Grouped partition bug and repair

The original 5.0.6 compiler appends GROUP BY and HAVING before wrapping the inner
select with its per-parent ranking filter. C++ returned from the partition branch
before those clauses. The [initial differences](parity/sql_baselines_5_0_6_initial_gaps.json.gz)
retain all 12 mismatches: missing HAVING SQL, its fourth binding and its Masked
parameter policy. Offset/limit variants include zero and saturating unsigned
bounds. The original SQL source explicitly describes grouping before ranking.

`SqlDialect::compile_select_sql` now keeps GROUP BY and HAVING inside the ranked
select, preserving their compilation/validation and binding order. Ordinary
ordering and slicing remain after the partition branch. No query, service,
executor or transport abstraction is removed or merged.

A new observer invokes the original compiler and SQLite provider on the same
seeded database as C++. Its 146 inputs cover HAVING thresholds 1/2/3/5, offsets
0/1/2, omitted/zero/one/two limits, both/all owners and two invalid grouped fields.
There are 144 actual SQLite executions and two compile-error observations.
Complete SQL, bindings, policies and compact rows match.

For example, after `COUNT(*) >= 2`, offset 1 and limit 1 select the second group
of each owner: owner 11 has green/count 2, owner 22 has blue/count 2; both ranks
are 2. This verifies grouped counts and per-owner paging rather than merely
checking a SQL substring. The C++ observer disables the default PMR resource,
allocates its provider/connection through UserContext and actually destroys each
User after serializing its result.

See the [source/native replay record](parity/sql_baselines_5_0_6.json) and
[grouped-partition input matrix](../tests/parity/sql_grouped_partition_inputs_5_0_6.json).
The kernel unit also checks retained HAVING bindings and grouped-field rejection.

## Historical execution adapters still need migration

Four historical development adapters do not compile against the new request
protocol: `executor_defaults_scope_oracle`, `sql_stream_oracle`,
`sql_query_oracle` and `sql_execution_oracle`. The original API now requires
QueryIntent and separates MutationCommand from its intent-bearing MutationRequest.
Old QueryRequest.comment fields and MutationRequest enum constructors are retired.

The [initial compiler output](parity/sql_baselines_5_0_6_original_protocol_compile_failure.json)
and [fresh structured replay](parity/sql_baselines_5_0_6_protocol_compile_failure_replay.json)
retain nine E0560/E0599 errors across those four byte-preserved files. These are
unmigrated test adapters, not defects in the original runtime or accepted runtime
observations. Their six historical execution/default corpus bindings remain open.
The observer build selects the six runnable binaries explicitly; it does not
claim that an all-bin historical observer build passes.

Next, migrate the adapters to actual QueryIntent and MutationCommand/MutationRequest
constructors, preserve missing/blank-intent rejection, and compare their execution,
counter, diagnostic, transaction and stream behavior. Any declared valid-intent
control inputs must remain separate from the unchanged historical inputs. Existing
5.0.6 SQL execution matrices keep their own original scope and evidence.

## Verification and remaining integration

Native passes 10/10, canonical modules 10/10 and targeted ASan/UBSan 2/2.
All 7,040 source/native outcomes reproduce, including the 146 additional controls;
the same complete matrix reproduces through module consumers. Module verification
checks 459 canonical consumer/fixture bodies and 1,479 exports. Pinned format/check
covers 1,352 handwritten C++ files. Sanitizer leak scanning is disabled.

The [terminal record](parity/sql_baselines_5_0_6_terminal.json) retains these scopes.
Eight failures from the original full integration have selected successful
follow-ups, leaving 43 recorded baseline failures unresolved. The fresh native
registry and [static inventory](parity/active_corpus_baseline_after_sql_baselines_5_0_6.json)
are not complete-suite acceptance. The original 472/549 result remains historical
and failing. No fresh full native, MIPS/QEMU or physical-device run is claimed.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_sql_baselines_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(sql_(dialect_contracts|dialect_helpers_rust_parity|dialect_in_rust_parity|transport_rust_parity|schema_rust_parity|mutation_rust_parity|dialect_hooks_rust_parity|dialect_rust_parity|grouped_partition_rust_parity)|sql_projection_scope_rust_parity)$'
python3 scripts/verify_sql_baselines_5_0_6.py \
  --cpp-prefix teaql_module_ --build-dir build_modules_native/modules
ctest --preset modules_native --output-on-failure \
  -R '^module_(sql_(dialect_contracts|dialect_helpers_rust_parity|dialect_in_rust_parity|transport_rust_parity|schema_rust_parity|mutation_rust_parity|dialect_hooks_rust_parity|dialect_rust_parity|grouped_partition_rust_parity)|sql_projection_scope_rust_parity)$'
ctest --preset sanitize --output-on-failure \
  -R '^(sql_dialect_contracts|sql_grouped_partition_rust_parity)$'
```

Build the selected C++ targets first. The replay builds its selected original
binaries with offline locked dependencies. `--record` writes separate active
corpora only after every declared comparison passes. Generation and original
generator sources are unchanged by this kernel repair. Broader generated
configuration, request protocols, provider composition and full phase exit gates
still require their own qualification.

Current follow-up: [SQL protocol qualification](sql_protocol_5_0_6.md) supersedes the pending current-adapter integration above. It retains the original retired-syntax files/compiler diagnostics while adding runnable current observers and separate valid-intent controls. The original eight-family acceptance remains unchanged.
