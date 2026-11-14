# Generated reverse-relation types through SQLite

This G14/AP02 continuation uses runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`, the locked Rust 5.0.6 references.
It carries string-encoded relation types beyond the earlier
[descriptor and compiler matrix](ap02_reverse_relation_type_composition_5_0_6.md)
into generated runtime assembly, actual SQLite relation loading, E reads and
JSON output. Complete AP02/G14, G13/G17 and P0-P7 retain their other exits.

The [runner](../scripts/verify_ap02_reverse_relation_sql_5_0_6.py) derives three
inputs from retained `type_composition_0086`. It keeps the source's `Self` type,
removes the explicit owner-identity override for the canonical Item query, and
sets the child's scalar `owner` as the foreign key. Two variants change the JSON
type to `crate::Child`; one also selects many cardinality. Fresh execution of
the unchanged original parser and current Java adapter establishes every
derived input's complete metadata effects before generation.

| Generated model | Relation descriptor | JSON type | Actual loaded result |
| --- | --- | --- | --- |
| `self_one` | Child, to-one | Self / Item | The graph contains Child; the mismatched typed JSON lookup omits `links`. |
| `child_one` | Child, to-one | crate::Child | The first ordered child becomes an object; an empty relation becomes NULL. |
| `child_many` | Child, to-many | crate::Child | Both ordered children become a list; an empty relation remains an empty list. |

Every model executes five queries: an unselected relation, a loaded relation,
an empty relation, a minimal child projection and a missing parent. The
[execution receipt](parity/ap02_reverse_relation_sql_execution_5_0_6.json.gz)
records **3/3 models and 15/15 original/C++ query results** with zero
differences. Both sides generate full models, install their generated runtime
assembly and execute through runtime/data-service/SQL/SQLite layers. All **32**
source, generation, compiler and execution processes exit successfully.
The original consumer resolves offline, verifies registry identities against
the reference lock, and then compiles with `--offline --locked`.

The comparison retains exact SQL statements, comments, purposes and result
counts alongside graph presence, load state, E values and JSON. It performs
no output normalization. The parent name and loaded child secret are read with
generated E methods. A minimal child projection gives the explicit NotLoaded
field result, and its JSON contains only the loaded `id` and required `owner`
key. The to-one case fetches two matching rows and retains the first ordered
child, matching the reference. Each selected relation uses one parent and one
child SQL query; E and JSON reads add no SQL. A missing parent uses only the
parent query and returns an empty result.

All **795 execution inputs**, including **261 local runtime headers**, remain
unchanged. The record pins the actual native compiler, native archives,
compiler command input, complete original/current Java producer sources,
observer sources and reference locks. The three original and three C++
generated file sets remain unchanged during compilation and execution.
The C++ observer disables the default PMR resource, closes each UserContext
after its output is consumed, and requires zero remaining User bytes. All
task-owned source, generated library, Cargo target and native output files are
removed after terminal exit: **422,343,134 measured bytes**. Stable native build
outputs and shared dependency downloads are retained.

The [permanent verifier](../tests/parity/verify_ap02_reverse_relation_sql_receipt_5_0_6.py)
replays the actual process outputs and binds them to those exact inputs and
generated models. Its [control receipt](parity/ap02_reverse_relation_sql_controls_5_0_6.json.gz)
rejects ten corrupted variants. Controls include truncated output and changed
source hashes, plus synchronized changes to both languages that falsely mark
a loaded relation as unloaded or add an implicit SQL query. Agreement alone
cannot pass these explicit invariants.

Selected native CTest passes **2/2** under ordinary Python and **2/2** under
`PYTHONOPTIMIZE=1`, covering the preceding type corpus and this execution
receipt. Pinned formatting checks **1,813** handwritten C++ files; active
module-path verification passes. The current registry contains **1,088**
tests and **606** distinct Python entry scripts; **77** retain **680** direct
`ast.Assert` nodes. This registry count is enumeration. No full 1,088-test
native epoch has run.

The [compact recipe diagnostics](parity/ap02_reverse_relation_sql_recipe_failures_5_0_6.json.gz)
retain the initial lock-resolution failure, an incomplete inheritance SQL
fixture and observer API errors. Those attempts ended before comparison and
their temporary builds were removed. An
[earlier successful execution](parity/ap02_reverse_relation_sql_before_header_inventory_5_0_6.json.gz)
already matched all 15 results but omitted the complete native-header input
inventory. Its [earlier controls](parity/ap02_reverse_relation_sql_before_header_inventory_controls_5_0_6.json.gz)
retain that narrower provenance scope. The final receipt above comes from a
fresh complete execution after adding header and compiler identities.

With the development toolchain activated and Maven available on PATH, reproduce
the actual run with a fresh receipt filename:

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_ap02_reverse_relation_sql_5_0_6.py \
  --record /tmp/ap02_reverse_relation_sql_fresh.json.gz
```

This gate qualifies the selected relation-read configurations. Other AP02
type/path/binder entry contexts, protected write and publication configurations,
remaining SQ02 provenance and the final current-source integration epoch keep
their separate acceptance requirements.
