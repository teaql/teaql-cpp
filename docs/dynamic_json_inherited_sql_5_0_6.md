# Physical inherited JSON query qualification

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This closes the physical inherited-JSON row of the G10 requirement matrix;
broader protected-storage/policy composition and final integration remain open.

## Actual execution boundary

The unchanged complete generated inheritance-graph domain and its C++ counterpart
execute JSON-translated requests through their generated runtime, registered
data service/executor and real SQLite provider. The existing test schema contains
School, Academy, ResearchAcademy, Specialist, Platform and Enrollment data.
Each inherited table has the original merged scalar storage; the fixture is
independent of generated schema/bootstrap and graph-save acceptance.

All **4,236/4,236** original/native outcomes agree without query, result, parameter,
SQL or error normalization. School, Academy, ResearchAcademy and Enrollment
cross ordinary/minimal projections, rows/count/exists, ordering/page replacement,
scalar/null/list/object operands, unknown paths, malformed/non-object JSON and
duplicate keys. Conditions include inherited name/rank, inherited
advisor aliases, ancestor campus code, budget and nested advisor/platform paths.
The known `enrollment_list` path exercises the actual reverse collection name.

Data distinguishes live/deleted roots and advisors, a loaded NULL advisor and a
missing non-NULL target. Forward-path membership follows the original child
live-row filter rather than synthesizing a matching target. Complete query Debug,
ten compact-row slots, scalar results and ordered safe SQL/parameter/intent
records are compared. This is actual request/provider execution, not a local
graph traversal or an inventory of generated names.

The matrix retains **3,840** successes, **396** construction panics and **3,840**
safe SQL records. Rejected condition construction does not execute SQL. Source
and database fixture hashes plus original registry versions/checksums are checked.
The initial original observer macro had an unclosed delimiter; its compiler
failure is archived separately and contributes no source execution evidence.

## Ownership and remaining scope

Default PMR allocation is disabled. Scoped input/query/result values leave scope
before actual UserContext deletion, which must retain no User bytes. SQLite
connection cleanup uses the existing Context resource mechanism. No runtime,
provider, generator or generated function changes in this checkpoint.

Native, imported module and ASan/UBSan outcomes are retained in the
[terminal record](parity/dynamic_json_inherited_sql_5_0_6_terminal.json), with leak
scanning disabled; each selected profile passes **1/1**. The canonical module
verifier covers **658** consumer/fixture bodies and **1,485** exports.
Pinned formatting/check covers **1,442** C++ files. No Java/
generation refresh, full-suite, MIPS or physical-device run is inferred.

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_dynamic_json_inherited_sql_probe
ctest --preset native --output-on-failure -R '^generated_dynamic_json_inherited_sql_rust_parity$'
```

This supplements the preserved [852-omission recipe repair](dynamic_json_inheritance_5_0_6.md).
The [G10 matrix](dynamic_json_qualification_matrix_5_0_6.md) retains protected
storage/readonly/facet/privacy composition and complete integration as separate
requirements; physical inherited JSON agreement alone does not complete G10/P5/P7.
