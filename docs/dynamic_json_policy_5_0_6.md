# Generated dynamic JSON queries and RequestPolicy

This checkpoint qualifies the actual generated JSON filter and execution paths
against locked Rust 5.0.6. It covers RequestPolicy observation, rewrites, rejection,
retained SQL diagnostics and trace framing. It does not establish complete G10,
G17 or P0-P7 acceptance.

## Original responsibility and runtime correction

The complete original generated domain is unchanged. Its public requests execute
through the original UserContext, registered domain executor and SQLite provider.
The oracle installs an application RequestPolicy through the public Context SPI;
the C++ observer uses the corresponding Context registration and generated APIs.
Each input gets a fresh Context and in-memory database using the retained explicit
schema and dataset. The known original schema-bootstrap failure is not bypassed
and relabeled as successful bootstrap qualification.

Original `ResolvedEntityDataService::prepare_select_query_owned` binds validated
intent, invokes behavior and then RequestPolicy, and passes the rewritten query
to execution. Original generated `authorize_query` removes the last Purpose frame
before the runtime wrapper appends its replacement. Earlier ancestry is retained.

The first expanded comparison found **1,344/3,072** differences. Count, exists and
page results were correct, but the native derived-query helper constructed another
PurposedSelectQuery without replacing its existing Purpose frame. Policy saw an
extra frame. The complete failures are retained in
[the initial comparison](parity/dynamic_json_policy_5_0_6_initial_gaps.json).

Shared runtime `reauthorize_generated_query` now replaces that last frame when
authorizing count/exists and page-row derivatives. The public wrapper's ordinary
constructor semantics remain intact. Application queries retain their original
snapshots. Generator templates and generated artifacts are unchanged.

## Actual execution matrix

The **3,072/3,072** source/native matrix comprises four generated models (Person,
Order, Product and Line), eight JSON filter sequences, four policies, six execution
forms, full/minimal selection and enabled/disabled SQL diagnostics.

- Filters include empty requests, ordinary scalar matches and misses, numeric
  ranges, multi-hop relation membership, explicit NULL matching, slicing, chained
  JSON updates and malformed input.
- Policies allow, append an ID restriction, append that restriction and replace
  slicing, or reject with the exact application policy error.
- Execution uses list, one, stream, count, exists and ordinary count-first page.
  Streams consume and close their cursor, then attempt an ordinary query using
  the same registered executor and policy.
- Observations retain the caller snapshot, policy invocation order, before/after
  filters and projection, aggregate definitions, comment/purpose and complete
  policy trace frames. Results include IDs/versions, totals, counts, existence,
  errors and stream reuse.
- Retained diagnostic observations compare SQL, parameter Debug values, intent
  and complete trace frames. Timing is not part of this deterministic observation.

The corpus contains **2,016 successes**, **672 policy errors** and **384 original
JSON-format panics**, with **3,472 policy calls**, **1,344 retained SQL logs** and
**1,136 returned entity rows**. Malformed JSON never reaches policy or SQL.
Rejected requests have no retained SQL logs or SQLite SELECT-authorizer callbacks,
including with diagnostic output disabled. Ordinary pages invoke policy on count
before rows; policy rejection prevents the next stage.

The installed SQLite authorizer counts SELECT authorization callbacks, not rows,
executed statements or elapsed work. Both sides observe **5,040** callbacks across
the entire matrix. Fresh databases and callback installation before the first
application query make zero callbacks useful rejection evidence. The callback is
removed before its retained observation or connection is destroyed.

The only comparison adaptation is the existing exact quoted Line `order` column
Debug spelling, applied to caller and policy snapshots. SQL, errors, values,
aggregate definitions and trace frames are compared without normalization.
Original lazy versus native startup stream delivery remains the documented C++
ownership adaptation; the matrix compares completed logical outcomes.

## Ownership and remaining scope

The native observer disables the default PMR resource. Policy state, nested
observations, requests, streams and database resources belong to UserContext and
are destroyed for every input. SQLite callbacks cannot extend Context lifetime.

The fixture uses ordinary nonsensitive fields. These observations qualify the
safe retained logging path for this model; they do not establish the complete
credential/sensitive-binding/privacy matrix. Inherited, constant, candidate and
temporal model combinations, additional policy/behavior composition, relation
loading/facets, deferred policy resources and final integration remain separate
obligations. All complete capabilities remain partial.

## Reproduction

Terminal validation passes native **8/8**, targeted ASan/UBSan **3/3** with leak
scanning disabled, and canonical module imports **2/2**. Live original replay
reproduces the new 3,072 observations and the unchanged 2,240 execution / 1,181
translation corpora, including the separate original schema failure. A known
wrong policy error proves that the comparator exits 1 on a real difference.
Pinned formatting/check covers 1,022 handwritten C++ files; 161 canonical
consumer bodies, 1,479 catalog exports and seven acyclic package modules verify.
No generator source change requires regeneration or a Java test gate here.

```sh
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_generated_dynamic_json_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --offline --bin generated_dynamic_json_5_0_6
python3 tests/parity/generate_dynamic_json_policy_5_0_6.py \
  --cpp build_native_current/teaql_generated_dynamic_json_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_dynamic_json_5_0_6 \
  --check
ctest --preset native --output-on-failure \
  -R '^(dynamic_json_policy_rust_parity|dynamic_json_execution_rust_parity|generated_dynamic_json_rust_parity|generated_execution_context_contracts|deferred_plain_page_rust_parity|deferred_plain_page_context_contracts|deferred_id_set_page_rust_parity|deferred_id_set_page_context_contracts)$'
```

Terminal gate evidence and source hashes are recorded in the accompanying
[verification record](parity/dynamic_json_policy_5_0_6.json) and
[source review](parity/dynamic_json_policy_5_0_6_source_review.json).
