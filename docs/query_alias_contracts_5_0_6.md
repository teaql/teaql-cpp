# Original result aliases and keyword field identities

Reference runtime: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Reference generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This repair addresses demonstrated G07/G08 alias errors and a G06/G14 identity
boundary. It does not establish complete general-query or metadata acceptance.

## Reproduced differences

The unchanged original generator emits a complete domain with fields named
`class`, `operator`, `template`, `type`, `match`, `ordinary` and an optional
`namespace` FK. Both original and initial native SDKs compile. The real generated
runtimes execute the same existing SQLite rows; observers do not implement query
compilation, persistence or row decoding.

The initial matrix disagrees on **328 of 660** outcomes. There are **184** complete
query construction differences from default aliases, and **216** source keyword
metadata failures; those sets overlap. The initial complete original/native
outputs and both pre-repair native SDKs remain unchanged.

C++ escaped member names had leaked into result keys: `class_count` became
`model_class_count`, and `sum_class` became `sum_model_class`. Rust's own keyword
escaping independently makes a default key such as `sum_type_field`. Explicit
aliases are separate input data and must not be rewritten. Assist additionally
converted every default aggregate key to camelCase, including ordinary `sum_id`.

## Separate responsibilities

Default aggregate keys now come from the locked Rust naming helpers. C++ member
and method spelling remains unchanged, including `sumModelClass()`. Numeric
function-selection defaults retain their different source rule: the canonical
field itself is the alias. Caller-provided aliases remain untouched.

Assist presents result keys as quoted string literals. Its identifier conversion
preserves those literals, so `sumModelClass()` documents `"sum_class"` and
`sumType()` documents `"sum_type_field"`. The emitted method remains camelCase.

The original generated struct's derived descriptor names a keyword field
`type_field`, while its query projects and filters `type`. These are different
source identities. The shared runtime constructor can now use an optional
model-provided `QUERY_PROJECTION_FIELDS` table separately from descriptor
property names. Models with that source difference receive the table; ordinary
models retain the existing descriptor-driven path. Metadata retains the original
Rust member identity and physical column. Query fields are not silently renamed,
so actual original unknown-field failures remain failures in C++.

The generic runtime constructor and its lifetime control pass before the new
metadata adapter is emitted. No executor, data-service, transport or provider
responsibility is moved into generation, and no allocation scope or dependency is
added. The explicit projection table contains immutable metadata literals.

## Actual source and integration evidence

All **660/660** final observations agree, including complete SelectQuery Debug,
actual compact result keys/Values and complete errors. The matrix crosses nine
aggregate methods for six numeric fields, an object FK count, normal/minimal
requests, repeated calls and default/ordinary/embedded-zero aliases. It preserves
**168** successful outcomes/rows, **216** original unknown-field errors and
**276** other original provider errors. A failed attempt is not counted as a
successful query.

C++ observers disable default PMR, register the SQLite connection in UserContext,
destroy observation values before deleting UserContext and require no retained
User bytes. The runtime-only control additionally verifies explicit Global
retention/rebinding and expired ordinary wrappers. Module consumers use the
canonical test sources; selected ASan/UBSan checks run with leak scanning off.

Final relevant native checks pass **15/15**, imported new checks **2/2**, and
selected ASan/UBSan checks **2/2**. Compiler controls pass **7 positive / 12
negative** contracts. Java passes **65/65** across generation, field layout and
JSON metadata. New positive/negative compiler fixtures use the actual generated
keyword domain. No full suite, MIPS or physical-device run is claimed.

## Regeneration and change proof

All **24 ordinary workspaces / 288 files** and **50 existing contract SDKs**
strictly reproduce. The additional keyword SDK and unchanged complete Rust SDK
also reproduce. This is regeneration through the real producer, not manual
artifact editing.

A development-only metadata probe calls the existing naming helpers to derive
allowed alias changes. Full old/new file comparison permits only the exact
literal repairs, conditional projection table/descriptor identities and their
specified guide lines. Explicit-alias methods and unrelated generated bodies
must remain byte-identical after pinned formatting. The original source/SDK
snapshots, complete restricted-update reports and initial failures are retained.

Among ordinary workspaces, only the checker-clock scalar query header changes
code; all 24 Assist guides correct quoted literal keys. The 50 contract SDKs have
zero code changes and 36 guide updates. The new keyword SDK has its separately
proved alias and identity changes. Original Rust artifacts remain unchanged.

Reproduction commands:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_query_alias_probe teaql_query_projection_identity_tests
ctest --preset native --output-on-failure \
  -R '^(query_alias_rust_parity|query_projection_identity_contracts)$'
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
python3 scripts/generate_query_alias_contracts_5_0_6.py --check
```

The [terminal archive](parity/query_alias_terminal_5_0_6.json.gz) records actual
commands, process exits, artifacts, preparation failures and complete logs. Source
field-template acceptance is tracked separately by the
[G07 requirement matrix](predicate_projection_requirements_5_0_6.md).
