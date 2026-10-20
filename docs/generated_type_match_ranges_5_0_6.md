# Generated type-matching input ranges: Rust 5.0.6

The general Q audit found a concrete input-contract omission: original
match_types accepts IntoIterator elements convertible to Value, while C++ exposed
only a Value span. Runtime EntityQuery now accepts constrained input ranges and
generated matchTypes delegates once to that existing Context-bound runtime path.
The Value-span interface and existing method bodies remain unchanged.

## Source responsibility and ownership

The original ordinary Person/Order request bodies append an internal_type IN
predicate after Value conversion. No model-type validation or discriminator
schema construction occurs in that method. The [source review](parity/type_match_ranges_source_review_5_0_6.json)
retains complete method bodies and their source lines/hashes. The reference is
runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The runtime reuses criteria_values and the existing Expr::in_list construction,
so empty/duplicate/NULL values and membership limits retain their core semantics.
Arrays, vectors, spans, transformed views and single-pass ranges work when their
elements construct core Value in an explicit resource. Unsupported elements and
scalar arguments fail in constraints. Existing C++ native integral/character
conversions retain their language bindings; this does not assert that Rust char
or a bare string implements the original IntoIterator input bound.

Nested values copy into the request's bound Context. Caller storage may then be
modified or destroyed. Existing kernel borrows must be alive during capture;
no hidden owner extends a caller Context lifetime. The lvalue/rvalue generated
adapters preserve forwarding and return category, take no extra Context argument
and add no SQL/provider behavior.

## Actual original and C++ evidence

All **198/198** actual original/native observations match: **192** constructions
and **six** SQLite attempts. Full/minimal Person and Order requests cover repeated
application, Value vectors and lazy views, native integer/string carriers and
single-pass streams. Values include empty and duplicate collections, NULL/typed
NULL, Boolean/numeric/Decimal/date/timestamp/JSON values, Unicode/NUL text and
511/512/513/520 unique elements. The observer uses the unchanged original generated
domain with reference-registry versions/checksums.

Every physical attempt preserves the original `SQL compile error: unknown field:
internal_type` failure for this ordinary model fixture. This is a discriminator
metadata boundary, not successful type-filter SQL or a reason to invent a model
column. Existing inherited/type-group qualification remains separate.

Native ownership controls disable default PMR, delete caller storage before
inspecting captured values, consume a single-pass stream once, keep Value and
native-integer spans callable, and inspect an explicit Global copy after deleting
the destination UserContext. Unsupported element types reject at compile time;
Business-ID request facades also preserve the general type-matching method.
This general query helper does not bypass their distinct field predicates.

## Generation and retained code proof

Java regressions pass **56/56**, including both range return categories, the old
Value span and accurate Assist. All **24 workspaces / 288 files** reproduce.
The controlled SDK update regenerates all **44** complete SDKs. It permits only
**134** exact constrained matchTypes delegates across **30** query headers and
**30** Assist updates; removing only those verified forwarding bodies and formatting
the remainder produces byte-identical old code. Strict SDK replay then reports
zero code/instruction changes. Original Rust and historical corpora are unchanged.

Compiler controls pass **one positive / two negative** contracts for primitive
arrays, Value spans, unsupported elements and non-range scalar arguments.
Declaration inventory remains 755 names and is not presented as semantic proof.
The new capability is an input overload of an existing method.

## Reproduce and remaining scope

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_type_match_ranges_probe teaql_generated_type_match_range_tests
ctest --preset native --output-on-failure -R '^generated_type_match_range'
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
```

Native checks pass **3/3**, import checks **2/2** and ASan/UBSan **2/2**, with
leak scanning disabled. Format/check covers **1,448** files; **664** canonical
consumer bodies and **1,485** module exports verify.

Import and sanitizer targets use the same observer and ownership bodies in their
separate profiles; module names have the `teaql_module_` prefix. See the
[terminal record](parity/type_match_ranges_5_0_6_terminal.json) for completed gates.
Final sanitizer leak scanning remains disabled.

The [general surface review](query_surface_review_5_0_6.md) retains the complete
G06/G07/P4/P7 requirements. This closes the demonstrated match_types iterator
omission, not full general-query, model/configuration, policy/privacy, pagination
or phase integration. No full-suite, MIPS or physical-device run is inferred.
