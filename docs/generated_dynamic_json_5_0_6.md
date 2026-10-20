# Generated dynamic JSON queries: Rust 5.0.6 checkpoint

Status: G10 partially implemented and qualified for the recorded model. P0–P7
and the complete generator capability matrix remain active.

## Fixed source and application contract

Runtime v5.0.6 is pinned at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The producer is `45e1572c4956ca963410d8ea436ead3c8857087a`; its complete,
unmodified dynamic_json fixture contains 50 files, including provenance.
The C++ producer emits thin model recipes over shared runtime translation:

```cpp
auto Q = user_context.queries<cpp_dynamic_json::query_factory>();
auto request = Q.orders().filterWithJson(
    R"({"customer.name":"Alice","lines.product.sku":"B","_page":2,"_pageSize":3})");
auto result = std::move(request)
                  .comment("what: inspect matching orders")
                  .purpose("why: review customer orders")
                  .executeForList(user_context);
```

Self fields use authentic storage names. Forward object names and foreign-key
aliases share the same property. Dotted forward, reverse one-to-one, reverse
many and recursively composed relations build membership subqueries with the
actual relation keys. Child queries retain the source live-version filter even
when their final field is unknown; an unknown top-level field contributes no
filter. Only top-level underscore-prefixed controls are skipped as criteria.
Non-object JSON contributes nothing. String criteria become contains patterns,
null sentinels remain distinct from ordinary strings, numeric pairs become
ranges, and other arrays follow the original field-operator rules. Empty arrays
can fail with the original required-value diagnostic. Ordering accepts source
string/object/array forms and only self fields; string ordering is descending.
Pagination preserves control precedence, positive page numbering, default size,
existing slicing and saturating offsets.

## Ownership and supporting core changes

Recipes never own Context data. Parsed JSON, nested descriptors, query payloads,
strings, patterns and original LIKE operands use the bound allocation scope.
Global retention and cross-Context composition require explicit destination
copies. Ordinary query wrappers expire after UserContext deletion and are safely
destructible. Context-borrowed format errors retain the original input; `what()`
returns a fixed safe prefix, while `to_string(destination)` checks the diagnostic
borrow. An explicit Global diagnostic copy can survive deletion of its source.

SelectQuery now retains readonly property definitions, facets and relation
aggregates through cloning/equality and request-metadata assembly. This restores
source-shaped nested query state; retention alone does not qualify complete
facet execution, readonly result hydration or the G08/G16 capability matrices.
Definitions are explicitly copied instead of extending a UserContext lifetime
through source-style shared ownership. Definition fingerprints remain the
existing C++ implementation contract; this checkpoint does not qualify nonempty
schema hash/debug equivalence.

Rust 5.0.6 `Expr::LikePattern` is distinct from a raw pattern value. All typed
contains/begin/end and negative counterparts retain both pattern and original
operand. SQL compilation validates their relationship before registering original
operand privacy. Standalone patterns, arbitrary raw LIKE and rewritten patterns
do not acquire inferred originals. Cached plans remove invocation redactions;
cache hits collect current operands, preserving nested binding offsets, then
rebuild invocation-local intent privacy from cached field policies. The readonly
schema participates in plan matching as in the fixed source. No new production
dependency was introduced.

## Source observations and declared representation adaptation

The committed corpus contains **1,181** direct original/C++ observations,
including **40** actual generated typed SQLite list queries. It covers scalar
criteria, aliases, incomplete/unknown relation paths, recursive cycles, ordering,
paging, malformed input, duplicate keys, sequential composition and list
IDs/versions. Query filters retain full original Debug observations, including
nested descriptors, query state, LikePattern provenance and panic text.

C++ descriptors store raw SQL identifiers and the dialect quotes them. The
frozen producer stores the Line `order` column as the already quoted `"order"`.
The comparator applies exactly that documented descriptor-debug spelling
adaptation; it changes no predicate, property identity, relation key, query state,
error or result. All original observations are stored verbatim. 189 observations
require that spelling adaptation; the other 992 match without it. SQL fixture
hashes and full producer provenance are checked on replay.

Both probes use the same explicitly declared existing SQLite database with
foreign keys enabled, including deferred one-to-one references. They call actual
generated runtime assembly and typed query execution. They do not substitute
mock rows or implement filtering outside TEAQL.

The frozen generator's public `ensure_schema` fails on an empty database for
this fixture because its already quoted column spelling does not match SQLite
foreign-key introspection. Its complete diagnostic is recorded separately in
[the source failure record](parity/dynamic_json_schema_source_failure_5_0_6.json)
and replayed with the live oracle. The existing database fixture also supplies
the unique referenced key and deferred constraints needed by the reverse
one-to-one metadata cycle. Neither the original fixture nor its generated crate
was patched. This checkpoint makes no successful bootstrap claim.

## Verification and reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
python3 scripts/verify_generator_parity.py
python3 scripts/generate_examples.py --check
python3 scripts/generate_entity_references.py --fixtures dynamic_json --check
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin generated_dynamic_json_5_0_6
python3 tests/parity/compare_generated_dynamic_json.py --cpp build_native_current/teaql_generated_dynamic_json_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_dynamic_json_5_0_6
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo test --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin original_like_privacy_5_0_6
```

The last command runs all five unchanged original SQLite LIKE privacy tests
through a standard-library future driver, without adding oracle dependencies.
These original tests and C++ regression tests are separate evidence, not a claim
that every privacy configuration has paired output observations.

Native and sanitizer Context tests exercise null default PMR, transient inputs,
recursive nested allocator propagation, explicit Global copies, deleted source
Contexts, error borrows and facet/aggregate/readonly metadata retention. SQL
executor tests cover cache hits, rewritten/raw/non-LIKE expressions, nested
subqueries, all six typed text lowering variants under plain/masked/credential/
unclassified policies and explicit Global intent copies. Sanitizer leak scanning is disabled
as required by the project. Final gates completed after every compiler/linker exited successfully:

| Gate | Result |
| --- | --- |
| Relevant native query/request/runtime regression | 10/10 |
| Targeted ASan/UBSan corpus, Context, SQL and expression tests | 4/4; leak scanning disabled |
| Module import contracts | 1/1 |
| Frozen original SQLite LIKE privacy suite | 5/5 |
| Java generator/field-layout/naming/pluralization tests | 34/34 |
| Positive/negative application compiler contracts | 2 positive, 4 negative |
| C++ regeneration | 15 sets, 165 files reproduce |
| Original generated fixture reproduction | 50 files reproduce; frozen worktree clean |
| Formatting | 912 handwritten files checked |
| Export catalog | 1,456 exports across seven modules |

MIPS and physical-device execution are outside this checkpoint.

## Remaining qualification

The [complete G10 requirement matrix](dynamic_json_qualification_matrix_5_0_6.md)
now consolidates source obligations, scoped semantic evidence and remaining
inheritance/protected-storage/policy/integration requirements. The subsequent
[domain execution matrix](dynamic_json_domains_5_0_6.md) qualifies 5,910 actual
constant/candidate/Business-ID JSON outcomes without ID allocation/bootstrap.

The [subsequent scalar/optional/temporal SQLite checkpoint](dynamic_json_scalar_5_0_6.md)
qualifies 5,088 additional outcomes and replays this complete corpus plus one/
stream/policy matrices against original registry versions/checksums. Its explicit
stream-reuse uint64 stimulus matches the original Rust argument; production
generic conversion semantics remain unchanged.

The [inherited recipe repair](dynamic_json_inheritance_5_0_6.md) subsequently
qualifies six-model inheritance constructions and fixes 852 ignored ancestor
field/path differences. Its 2,656 exact outcomes and separate compiler/profile
gates supplement the historical records below without relabeling them.

- Additional supported model combinations: inherited fields/discriminators,
  constant/candidate fields and more scalar/temporal/optional types.
- Generated list/one/stream application integration beyond the recorded list
  executions; request policy rewriting and complete trace/privacy matrices.
- Source panic ownership/exception recovery adaptations across further failures;
  the C++ checked diagnostic model is deliberate, not identical Rust unwinding.
- Full facet/relation-aggregate execution and readonly schema result attachment;
  restored query metadata is a prerequisite, not complete G08/G16 acceptance.
- Generated schema/bootstrap limitations and complete assembly/sample-data gates
  remain under G12/G13 and the existing qualification matrices.

The full goal remains active; this scoped checkpoint does not waive those gates.
