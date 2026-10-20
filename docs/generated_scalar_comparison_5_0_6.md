# Generated scalar comparison Value inputs: Rust 5.0.6

The generator adds constrained ordinary-scalar comparison overloads over the
existing Expr/runtime mechanisms. This restores the original named Is/IsNot,
ordered comparison and Between operand-conversion responsibility without adding
a runtime algorithm or dependency. Typed field overloads remain present.

## Source branch and input semantics

The locked fieldMethods template separates version, Business-ID, Boolean and
ordinary scalar branches. Ordinary scalars accept values convertible to core
Value; ID fields omit ordered/Between methods in the source. The new C++
overloads follow those guards. Object references and Business IDs keep their
separate typed contracts; unsupported operands fail in constraints.

Single operands require constructible_from<Value, Memory, const Input&>.
Between checks Lower and Upper independently. The thin delegate constructs its
existing Expr in the bound allocator, then passes it to andFilter. This keeps
literal payloads scoped and independent of caller storage. Both lvalue/rvalue
fluent forms preserve binding; execution still receives only UserContext.

Core Value inputs retain their original variant, including explicit NULL and
TypedNull. For an operand previously accepted only through an implicit conversion
to the field parameter type, the new exact-match template can instead retain the
operand's core variant. This follows source Into<Value> semantics rather than
promising unchanged implicit coercion. Exact existing field-type overloads and
all old method bodies remain; Business-ID bypasses remain rejected.

## Actual unchanged original/native execution

A separate Rust observer compiles the untouched original dynamic_json library
against locked runtime fe82a18982415f1746df9a2cf7c42f5e9abdc6d3 and producer
45e1572c4956ca963410d8ea436ead3c8857087a. Reference registry versions/checksums,
model hashes and complete original-file provenance verify before replay.

The 2016-case matrix crosses six human/ordinary fields, full/minimal projection,
repeat, source-admitted operators and fifteen input variants. ID fields use only
source Is/IsNot. Other fields use equality, inequality, four ordered comparisons
and Between. Operands include ordinary/typed NULL, empty/Unicode/NUL text,
Boolean, signed/unsigned extrema, F64, Decimal, Date, Timestamp, JSON, List and
Object. Upper operands are independently typed. Every complete query Debug
compares directly; ninety-six cases execute actual original/native SQLite and
compare rows or full driver errors. Other cases remain construction evidence.

Native lifecycle tests disable default PMR, delete caller input Context after
capture, keep typed overloads callable, accept heterogeneous Between operands
and retain explicit Global copies after actual source User deletion. Owning
kernel values die before their allocators. Compiler controls reject unknown
operands and Business-ID Value/text bypasses; membership regression verifies the
previous generic-range adapters stay callable.

SDK recording removes only the exact new comparison delegate signatures and
bodies, formats the remainder and requires all old code to be byte-identical.
This includes explicit guards, lvalue/rvalue pairing and field/operator calls;
successful generation alone is not semantic evidence. Assist documents the
actual operand and ownership boundaries.

## Completed gates

The original/native matrix passes **2016/2016**, including **96** actual SQLite
executions. Native regression passes **8/8**, module consumers **2/2** and targeted
ASan/UBSan **2/2**; both selected consumers run the complete new matrix. Leak
scanning is disabled. Compiler controls pass **2 positive / 3 negative**, and
related Java checks **53/53**.

All **23 workspaces / 276 files** and **44** complete contract SDKs strictly
reproduce. The recorded update adds **1598** exact comparison delegates and
changes **30** Assist files; every previous generated body remains byte-identical.
Formatting checks **1424** handwritten C++ files and canonical module verification
checks **632** consumer/fixture bodies. The export catalog remains **1485**.
No full-suite, MIPS or physical-device result is inferred from these gates.

## Reproduction and remaining scope

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_scalar_comparison_probe teaql_generated_scalar_comparison_tests
python3 tests/parity/compare_generated_scalar_comparison_5_0_6.py \
  --cpp build_native_current/teaql_generated_scalar_comparison_probe
ctest --preset native -R '^generated_scalar_comparison_'
```

[Terminal evidence](parity/generated_scalar_comparison_5_0_6.json) records the
completed source/native/compiler/Java/module/lifetime and reproduction scope.
The [source surface review](query_surface_review_5_0_6.md) keeps the full named
G06/G07/P4/P7 requirements, including remaining model/projection/function/order
and governance interactions. This comparison repair does not complete a whole
capability or redefine the goal.
