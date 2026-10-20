# Query field branches and missing numeric/reference adapters

Current status: G07 is qualified by the [complete field-interface acceptance](predicate_projection_acceptance_5_0_6.md). Earlier checkpoint counts and open-scope statements below remain historical; G06 and other capabilities retain their own phase requirements.

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator reference:
`45e1572c4956ca963410d8ea436ead3c8857087a`. This checkpoint repairs demonstrated
generation omissions. It does not qualify complete G06/G07/G08/G12/G13 or P4/P7.

## Complete original fixture

The new `tests/generator/query_field_branches.xml` declares a Tenant root and an
Entry with ordinary/optional strings, text, password, Boolean, Decimal, double,
float, integer, date and timestamp fields. Original generation also supplies ID
and version. The complete unchanged original domain compiles against the locked
runtime and original registry versions/checksums. The nineteen generated Rust
files and provenance are retained in `tests/generator/parity/query_field_branches_rust`.
This avoids using a rootless domain that cannot compile as application evidence.

The original public field branches are observed directly. Version supplies
ordering; ID supplies equality/membership and ordering; Boolean supplies its
positive/negative and availability forms; ordinary scalars supply comparison,
range, membership, availability and ordering. Optional fields preserve their
distinct values and availability. The **8,316/8,316** exact complete query outcomes
cover ordinary/minimal entries, repeated application and eleven operand variants,
including NULL, typed NULL, Unicode with embedded zero and unsigned overflow.
These are construction observations, not database execution attempts.

## Reproduced omissions and correction

The initial correct-class declaration review found eleven missing public names:
ten float function/statistical projection methods and `filter_by_tenant`.
The earlier forward-declaration parser mistakenly inspected another class; its
archive is retained and excluded from acceptance.

The locked `SKYNETObjectAdaptor` resolves a nonempty metadata attribute before a
JavaBean method. Recognized Boolean tokens become booleans; other nonempty strings
remain truthy StringTemplate values. The primitive float inherits `isNumber=true`,
although `FieldDescriptor.isNumber()` omits float. Numeric aliases can inherit the
same flag. C++ generation now follows that attribute precedence before falling
back to the JavaBean method. Function projections, statistical methods and their
Assist guidance share this decision. Java checks additionally cover explicit
`isNumber=off` and a truthy non-Boolean attribute.

Ordinary object-reference requests now emit `filterBy...` through the existing
`request::EntityReference` and `entity_id_value` protocols. They delegate to the
existing Context-owned `Expr::eq` and `andFilter`; no reference conversion,
allocator, SQL execution or ownership algorithm is duplicated. Candidate-backed
references retain their distinct source public/private branch.

A supplemental **432/432** original/native matrix qualifies both numeric function
selection methods, eight statistical variants and reference filters with Value
and integer carriers. It covers ordinary/minimal requests, repeated calls, three
aliases including embedded zero, and IDs zero, seven and unsigned maximum. Full
SelectQuery Debug is compared without normalization. This does not establish
SQLite statistical-function support or execution of the newly generated float
methods. Existing projection/text SQL execution evidence remains separate.

The subsequent [actual SQLite execution checkpoint](query_field_sql_5_0_6.md)
qualifies all 432 restored-method request outcomes, including successful rows,
unsupported functions, invalid alias tokens and unsigned binding failures. Its
separate corpus and terminal record preserve this construction checkpoint.

## Guards, ownership and declaration scope

Eight actual original compiler failures each require exit 101, E0599 and the
specific missing method: Boolean generic filtering, ID before/function selection,
optional Boolean membership, password sound, string function selection, text
containment and version equality. Original generated files are not edited.
The selected C++ compiler gate passes two positive and five negative cases.

C++ retains some earlier typed convenience APIs beyond the source-selected
surface. The source text/password/version refusals do not establish rejection
of every such native extension. Newly constrained Value adapters preserve their
source branch boundaries. Complete typing and extension dispositions remain part
of the wider qualification review.

Both native observers disable default PMR allocation and actually destroy each
UserContext after its PMR values leave scope, requiring no retained User bytes.
Q requests and nested values use their bound Context; new delegates add no owner
and do not extend a borrow. Existing cross-domain retention contracts remain
unchanged.

The final reproducible inventory finds **755/755** original public request names,
including eight asynchronous execution methods, under the documented C++ naming
adaptations. The earlier 747-name review excluded asynchronous methods. Matching
names alone does not qualify their semantics, overloads, guards or execution.

## Verification checkpoint

Relevant native checks pass **5/5**, imported module checks **2/2**, targeted
ASan/UBSan checks **2/2** with leak scanning disabled, and Java regressions
**54/54**. C++ compiler checks pass **2 positive / 5 negative**; original refusals
pass **8/8** with method-specific diagnostics. Pinned formatting/check covers
**1,436** C++ files; module checks cover **644** canonical consumer/fixture bodies
and **1,485** exports across seven packages.

Twenty-four C++ workspaces / 288 files and all forty-four retained SDKs reproduce.
The SDK update adds **78** exact delegates across twenty-two query headers and
one Assist file. Existing function bodies remain byte-identical after removing
only the new delegate pairs. Constant requests retain their exact existing
selection-mutation wrapper. The initial verifier did not recognize that wrapper;
the failure and narrow verifier correction are preserved. Original Rust and
historical C++ SDKs remain unchanged.

Reproduction commands use the locked toolchain:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_query_field_declarations_5_0_6.py
python3 scripts/verify_query_field_guards_5_0_6.py
cmake --build --preset native --target \
  teaql_generated_query_field_branches_probe teaql_generated_query_field_extensions_probe
ctest --preset native --output-on-failure \
  -R '^generated_query_field_(branches|extensions)_rust_parity$'
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
python3 scripts/generate_entity_references.py --fixtures query_field_branches --check
```

Complete field/model configurations, physical execution of restored numeric
branches, combined governance/privacy and the remaining phase gates retain
their full requirements. No MIPS, physical-device or full integration run is
claimed. See the [terminal record](parity/query_field_branches_5_0_6_terminal.json),
[SDK proof](parity/query_branches_sdk_change_5_0_6.json) and
[source declaration inventory](parity/query_field_branch_all_public_declarations_5_0_6.json).
