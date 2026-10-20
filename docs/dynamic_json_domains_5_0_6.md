# Constant, candidate and Business-ID JSON query execution

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The [complete G10 requirement matrix](dynamic_json_qualification_matrix_5_0_6.md)
separates this evidence from the remaining capability/phase acceptance.

## Real domain execution

Two complete unchanged original-generated domains compile against exact
reference registry versions/checksums. Original and C++ programs use the same
existing SQLite fixture, generated runtime registration and actual query
execution. OrderStatus, Record, Person, Receipt and Ticket cover constants,
constant references, candidate phases, nullable candidate tags and two distinct
Business-ID definitions. No local filtering or fabricated query result replaces
the runtime/provider.

All **5,910/5,910** original/native outcomes agree without query, value, error or
SQL normalization. Inputs cross ordinary/minimal projection, rows/count/exists,
ordering/page controls, scalar/null/range/list/object operands, malformed and
non-object JSON, aliases and related entity paths. Complete construction Debug,
eleven row slots, scalar outputs and safe parameterized SQL logs retain original
semantics. The matrix records **5,520** successes, **390** condition panics and
**5,520** safe SQL log entries. Rejected condition construction does not execute SQL.

The original raw JSON translation applies ordinary operators to model storage
fields. It does not resolve a constant code into an ID, validate candidate
membership, allocate a Business ID or turn presentation input into write authority.
The paired calls preserve those distinctions instead of inventing business rules
in JSON translation.

## Startup, ownership and scope

Original generated Business-ID startup requires a key provider even though these
queries do not allocate IDs. The observer supplies a public deterministic all-zero
32-byte test key/version through the original startup environment contract.
The C++ observer assembles Context-owned query infrastructure through existing
module/executor/provider contracts; it reads no environment key and performs no
allocation or bootstrap operation. This is the previously documented C++ trusted
Context assembly adaptation, not a new production key configuration behavior.

The SQL fixture establishes existing data independently of ensure-schema/seed
acceptance. Both original domain file hashes and the SQL fixture hash are checked
on replay. C++ disables default PMR allocation, destroys scoped JSON/query/results
before actual UserContext deletion and requires no retained User bytes. SQLite
connections are registered resources, cleaned before arena reclamation.

The first C++ observer build needed a qualified generated `require_value` helper
when including two domains. That harness error is archived; production and
generated code were not changed. The new native, module and ASan/UBSan corpus
checks each pass **1/1** with leak scanning disabled. Canonical module checks
verify **657** consumer/fixture bodies. Pinned format/check covers **1,441** files.

## Reproduction

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_dynamic_json_domains_probe
ctest --preset native --output-on-failure -R '^generated_dynamic_json_domains_rust_parity$'
```

Use `compare_dynamic_json_domains_5_0_6.py --rust` with the locked original
observer to replay source results too. The comparator supplies only its fixed
test key and verifies original package/registry provenance. See the
[terminal record](parity/dynamic_json_domains_5_0_6_terminal.json).

Physical inherited JSON/policy/storage composition and complete P5/P7 integration
remain explicitly pending in the G10 matrix. No full-suite, Java/generator refresh,
MIPS or physical-device test is claimed for this observer-only checkpoint.
