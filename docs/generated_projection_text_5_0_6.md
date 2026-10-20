# Text predicates and numeric projection functions: Rust 5.0.6

Current status: G07 is qualified by the [complete field-interface acceptance](predicate_projection_acceptance_5_0_6.md). Earlier checkpoint counts and open-scope statements below remain historical; G06 and other capabilities retain their own phase requirements.

The generator restores ordinary string sound-like Value operands by emitting
thin constrained Expr/andFilter delegates. Existing text overloads and runtime
mechanics remain unchanged. Source-selected string predicates, numeric projection
functions, aliases/removal and ordering are compared through actual original
queries and SQLite executions rather than method counts.

## Source responsibility and guard boundaries

fieldMethods in the locked producer selects string predicates only for ordinary
string fields that are not large-text/password carriers. Contains, starts, ends
and their negatives accept text. Sound-like accepts Into<Value>; the new C++
overload requires Value-constructible operands and retains the original variant.
Existing typed and Business-ID guards remain. Payloads copy into the bound
Context; no extra Context argument, hidden owner/cache or runtime dependency is
added.

The source has no From<Option<String>> for Value. An actual unchanged generated
query rejects Option<String> with E0277; the complete original diagnostic is
retained. Native constraints likewise reject optional operands. NULL intent
uses an explicit supported Value::Null or TypedNull. This is a source refusal,
not an unsupported feature silently implemented with different semantics.

Numeric select...WithFunction delegates to aggregate construction, not an
expression projection. Default alias and explicit alias methods retain function,
field and order. Group aliases use independent expression projections. Unselect
removes only plain/raw field projections; independent aliases and aggregates
remain. Ascending/descending/GBK order paths preserve their declared shapes.
SQLite-specific unsupported aggregate functions remain execution failures. The
current default GBK compiler emits its argument unchanged: the marker API does
not establish GBK collation. Source and native paths preserve this exact behavior,
rather than inventing a database-specific conversion.

## Complete original/native observation matrix

The original generated dynamic_json library remains byte-verified at producer
45e1572c4956ca963410d8ea436ead3c8857087a and runtime
fe82a18982415f1746df9a2cf7c42f5e9abdc6d3. A separate source observer uses the
reference registry versions/checksums and original provider/runtime path.

The 4376-case matrix includes all seven text methods on human Name/ordinary Code,
full/minimal projection, repeated predicates, ordinary/empty/Unicode/NUL/wildcard
text, four order routes and explicit non-text sound operands. Numeric Priority
and Amount cases cross every one of twelve AggregateFunction variants, six
selection/raw/removal/grouping/repetition pipelines, three aliases and four
order routes. Every case attempts actual original/native SQLite execution and
compares complete query Debug, selected row observations or full errors without
normalization. This tests the stated two-domain configurations, not all model,
privacy, policy or provider integrations.

Native lifecycle checks disable default PMR, delete caller input Context after
sound capture, verify removal preserves independent aliases/aggregates, and
retain explicit Global copies after actual User deletion. Compiler controls
accept sound Value operands and reject optional scalar/sound inputs. SDK update
recording removes only exact new sound delegates and requires every old generated
body to remain byte-identical. Assist documents actual guard and failure rules.

## Completed gates

The complete original/native matrix passes **4376/4376**, with **2016** expected
SQLite aggregate-function failures preserved as full matching errors. GBK-marked
paths use the verified default pass-through compiler; they are not labeled GBK
collation evidence. Native regression passes **8/8**, module consumers **2/2**
and targeted ASan/UBSan **2/2**; both selected consumers execute the full matrix.
Leak scanning is disabled.

The corrected compiler harness passes **3 positive / 2 negative**. Initial
missing-header negatives are excluded from API evidence and archived separately.
Related Java checks pass **53/53**. Original Rust optional refusal retains actual
E0277 output. Generation and SDK evidence records the **174** exact new sound
Value delegates, **30** Assist changes and the later instruction-only GBK
correction; no old generated method body is changed. Formatting checks **1426**
handwritten C++ files; canonical module checks **634** consumer/fixture bodies.
The export catalog remains **1485**. All **23 workspaces / 276 files** and **44** complete contract SDKs strictly
reproduce after the guidance correction. No full-suite, MIPS or physical-device
result is inferred from these focused gates.

## Reproduction and remaining scope

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_projection_text_probe teaql_generated_projection_text_tests
python3 tests/parity/compare_generated_projection_text_5_0_6.py \
  --cpp build_native_current/teaql_generated_projection_text_probe
ctest --preset native -R '^generated_projection_text_'
```

[Terminal evidence](parity/generated_projection_text_5_0_6.json) records completed
gates and exact scope. Full G06/G07/G08/P4/P5/P7 requirements remain open,
including additional model/type/naming profiles, intent/privacy/governance and
relation/execution compositions. No existing phase requirement is waived by
this scalar/text matrix.
