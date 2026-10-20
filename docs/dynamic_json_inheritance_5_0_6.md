# Inherited dynamic JSON recipes and relation analysis

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This repairs an observed G10 generation defect and related inherited G08/G12
emission omissions. Full capability and phase acceptance remains separate.

## Reproduced omission

Original `ObjectDescriptor.getPredefinedFields()` merges inherited fields before
the model's own compatible fields. Original Rust naming helpers use that list
for dynamic JSON scalar/object lookup and object-analysis methods. The independent
C++ implementation workspace retains an older shared model method that returns
only the model's own fields.

Most C++ model/query generation already uses `CppFieldLayoutMetadata.fields`,
which implements the locked merge. Five remaining scalar/object field collection
calls used the older naming helper instead. Consequently Academy and
ResearchAcademy JSON requests ignored inherited name/rank/version fields,
foreign-key aliases and advisor/platform paths. Their relation grouping/count/
facet methods and related Assist enumeration also omitted inherited references.

The new complete original-generated inheritance-graph observer compiles against
the locked runtime and exact original registry versions/checksums. Its initial
comparison records **852** differences out of **2,656** inputs; every original
outcome and initial native difference is preserved unchanged.

## Generator correction

All five emission sites now use the existing canonical field-layout adapter,
then the original simple/object property predicate. This covers dynamic JSON
self-field lookup, chain recipes, object-analysis delegates and Assist enumeration.
The change adds no field merge, JSON translation, relationship traversal or
allocation algorithm to generated application code. Reverse-relation enumeration
retains its source-defined behavior rather than inventing inherited reverse names.

Three-level Java regression checks confirm ancestor scalars, ancestor object
aliases, chain targets, grouping details and counts in the leaf output. Existing
metadata tests retain compatible redeclaration, invalid inheritance and layout
ordering checks. The original generator checkout and other-language templates
remain unchanged.

## Actual semantic matrix

All **2,656/2,656** exact original/native full query constructions now agree,
without metadata, filter, order, error or projection normalization. Six original
models cover roots, ordinary entities, a child and a grandchild, optional advisors
and nested forward paths. Inputs cross ordinary/minimal selection and existing
slices; NULL, empty/Unicode/embedded-zero text, null sentinels, Boolean, numeric,
range/list/empty operands; ordering forms and paging precedence; malformed and
non-object JSON, duplicate keys and unknown paths. Unknown reverse-name spellings
are negative cases, not successful reverse traversal evidence.

The comparison uses original generated methods and runtime models. It does not
execute SQL or establish physical inherited grouping/facet behavior. Existing
typed-inheritance and ordinary JSON regression corpora remain separate, and are
rerun against rebuilt native consumers.

Default PMR allocation is disabled. Each native input/query/output leaves scope
before actual UserContext deletion, which must retain no User bytes. Borrowed
format diagnostics keep the existing checked lifetime and explicit destination
copy rules. No production runtime/SQL/provider function or ownership scope changes.

## Reproduction

```bash
source scripts/toolchains/activate.sh
python3 scripts/generate_examples.py --check
cmake --build --preset native --target teaql_generated_dynamic_json_inheritance_probe
python3 tests/parity/compare_dynamic_json_inheritance_5_0_6.py \
  --cpp build_native_current/teaql_generated_dynamic_json_inheritance_probe
ctest --preset native --output-on-failure \
  -R '^(generated_dynamic_json_inheritance_rust_parity|generated_inheritance_query_rust_parity|generated_dynamic_json_rust_parity)$'
```

Current compiler, Java, reproduction and profile outcomes are recorded in the
[terminal evidence](parity/dynamic_json_inheritance_5_0_6_terminal.json).
Native regressions pass **3/3**, modules **1/1**, ASan/UBSan **1/1** with leak
scanning disabled, compiler **1 positive / 2 negative**, and Java **55/55**.
All **24** workspaces / **288** files and **44** original fixture files reproduce.
Formatting/check covers **1,439** C++ files, **647** canonical consumer bodies
and **1,485** module exports verify.
The first SDK strict check rejected stale producer provenance after the code
change. Explicit provenance refresh still required every old SDK code byte to
match; all **44** complete SDKs remain unchanged and strictly reproduce afterward.
The first positive compiler stimulus lost JSON quotes through Python escaping;
its failure is retained, and the corrected stimulus uses a C++ raw string.

Complete candidate/constant/temporal/optional configurations, policy/privacy,
physical relation/facet execution, assembly and P4/P5/P6/P7 integration remain
required. No MIPS or physical-device execution is claimed.
