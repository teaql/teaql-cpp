# Generated scalar membership input ranges: Rust 5.0.6

Ordinary scalar IN/NOT IN methods now accept constrained C++20 input ranges,
matching the original IntoIterator<Item = impl Into<Value>> responsibility.
Existing typed span overloads remain unchanged. Generation delegates conversion
and expression construction to the already qualified EntityQuery::filterField
runtime; no new runtime algorithm or dependency is introduced.

## Source guard and ownership contracts

The locked request template emits scalar IN/NOT IN in the non-version,
non-Business-ID, non-Boolean scalar branch, including ordinary ID fields. Object
reference methods have separate contracts. C++ emits the new range overloads
only in that branch; existing native extensions are not relabeled as original
Rust declarations. Human/non-human predicate prefixes use the centralized
semantic naming service, and application APIs remain lowerCamelCase.

Both lvalue/rvalue overloads require input_range and elements constructible as
core Value in an explicit allocation resource. Arrays, vectors, spans, transformed
views and single-pass inputs work; unsupported elements fail in constraints.
Values copy into the request's bound allocator. Caller-owned inputs may then be
modified or destroyed. Ordinary core input values must stay alive during the
copy, as with existing kernel borrows. Business-ID ranges remain distinctly typed;
the new overload does not admit arbitrary Value or raw text into those methods.

The fluent adapter forwards the range once to the existing field-operator
implementation, which retains original deduplication, NULL, empty membership,
large-list and repeated-filter behavior. No additional Context parameter,
provider call, hidden cache or lifetime extension is added.

## Actual original/native evidence

The untouched original generated dynamic_json library runs in a separate Rust
observer with reference-lock versions/checksums. Six fields cover human Name/ID
and ordinary Code/ID/Priority/Amount. The 1188-case matrix covers full/minimal
requests, IN/NOT IN, repeated application, Value lists and lazy views, native
integer/string carriers and single-pass input. Values include empty/duplicate
lists, NULL/typed NULL, numeric/Boolean/Decimal/date/timestamp/JSON variants,
Unicode/NUL strings and unique 511/512/513/520-element lists.

All complete query Debug values compare without normalization. Thirty-six cases
also execute original/native SQLite compact-row queries and compare rows and
full errors. Other cases remain construction evidence. The original Source
collection can use the same values with different iterator forms; the native
stream is consumed exactly once in the lifecycle contract.

Default PMR is disabled. Native cases actually delete UserContext and require
zero retained User bytes. Runtime tests destroy caller Context inputs after
capture, keep existing typed spans callable and retain an explicit Global copy
across source User deletion. Compiler controls reject unsupported element types
and Business-ID Value/text bypasses. The initial observer typo is archived as a
preparation failure, not a source-library or semantic failure.

The controlled SDK update removes only exact constrained IN/NOT IN delegates,
checks their forwarding/field-operator bodies, formats the remainder and requires
all old generated bodies to remain byte-identical. Generated Assist documents
these actual input and ownership rules.

## Completed gates

The complete original/native matrix passes **1188/1188**. Native regression
passes **8/8**, module consumers **2/2** and targeted ASan/UBSan **2/2**, with
leak scanning disabled. Both selected consumers replay all 1188 cases.
Compiler controls pass **1 positive / 3 negative** and related Java tests
**53/53**. All **23 workspaces / 276 files** and **44** contract SDKs strictly
reproduce. The controlled SDK update adds **648** exact constrained delegates
and changes **30** Assist files; every old generated body remains byte-identical.
Pinned formatting checks **1422** handwritten C++ files, and canonical module
verification checks **630** consumer/fixture bodies. No full-suite, MIPS or
physical-device run is inferred from these focused gates.

## Reproduction and remaining scope

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_scalar_membership_probe teaql_generated_membership_range_tests
python3 tests/parity/compare_generated_scalar_membership_5_0_6.py \
  --cpp build_native_current/teaql_generated_scalar_membership_probe
ctest --preset native -R '^(generated_scalar_membership_rust_parity|generated_membership_range_context_contracts)$'
```

[Terminal evidence](parity/generated_scalar_membership_5_0_6.json) records native,
module, lifetime/compiler, Java and reproduction gates. The original fixture and
older field-criteria corpus remain unchanged. Full G06/G07/P4/P7 requirements,
including complete model/type/projection/function/order and execution governance
matrices, remain open. This input-conversion repair does not redefine their scope.
