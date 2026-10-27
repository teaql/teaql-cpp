# Canonical identity in generated E roots (Rust 5.0.6)

Status: EID01 is corrected and G11 is requalified. Installed original/native
960/960, ten effective result controls, Java 88, native 5, imported modules 3,
scoped ASan/UBSan 1 and four-positive/six-negative compiler contracts pass.
All 80 SDKs reproduce with only 188 canonical-root arguments in existing E
factories. Complete G13/G14/G17 and P0-P7 acceptance remain open.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.
The [source/body review](parity/generated_expression_identity_source_review_5_0_6.json.gz)
verifies the original canonical constructor, every initial differing field and
the unchanged complete runtime class/layout prefix.

The [initial comparison](parity/generated_identity_expression_before_v2_5_0_6.json.gz)
executes complete original SDKs against the locked Rust runtime and current
C++ SDKs from the identical models. The source template captures its canonical
model name in the root description. Descriptor/entity overrides have a separate
responsibility: `Item` can have `TeaqlEntity::ENTITY_NAME == "Other"` while
`E::item(value)` still reports `Item(id=1001)`. Current native runtime derives
the root from `T::ENTITY_NAME` and incorrectly reports `Other(id=1001)`.
Native entry spelling, field paths, loaded/NULL/value state and absence of
implicit SQL agree in the initial matrix.

The runtime-first target is a separate static canonical-root argument using the
existing `StaticString` contract. Existing exact one- and two-argument entity
factory signatures retain their previous fallback behavior. The new argument
is copied into the existing Context-owned root description. It adds no stored
member, ownership domain, allocation ID, singleton or dependency. Temporary
entities and runtime/local name buffers remain disallowed. Generated factories
then supply their canonical model name independently from their native API
entry. Application calls retain one entity argument.

The observer crosses Item and inherited Child, eight sparse/new/JSON states,
six scalar/nested read paths and eval/unwrap. Original source panic fields retain
their model identities; only the established C++ method spelling/English and
loaded-NULL message adapters apply. JSON decoding remains presentation input,
not persistence authority. Default PMR is disabled, actual UserContext deletion
is checked, and reading E must not execute SQL.

The [first observer attempt](parity/generated_identity_expression_before_5_0_6.json.gz)
compiled its Rust sources but failed while parsing a source human message:
the original renderer leaves embedded quotation marks unescaped. The corrected
observer removes only that field's outer quote delimiters and preserves its
embedded text; all actual runtime diagnostics remain unchanged. This controller
failure is separate from the subsequently reproduced native root mismatch.

## Installed runtime and generation

The shared runtime now accepts a third `StaticString` argument for the
canonical root, alongside its existing static native-entry argument.
Separate legacy overloads retain both exact entity-factory pointer signatures;
the existing scalar factory signature also remains usable. A deleted rvalue
counterpart retains temporary-entity refusal. Static metadata cannot extend the
Context or the entity lifetime.

The permanent runtime test captures an ID, changes the original entity's ID,
traverses a missing nested field, explicitly copies a scalar and an error into
the destination scopes, deletes the source UserContext, and checks retained
canonical/native names and rejection of stale access. Final destination deletion
returns all UserContext blocks. Default PMR is disabled. The kernel was qualified
before producer installation: native 2, scoped ASan/UBSan 1 and imported-module 1.
The compiler gate passes four positive and six negative contracts, including
both legacy signatures, static metadata, dynamic/local-name refusal and deleted
temporary binding.

The generator emits only metadata at the existing factory call:

```cpp
static ItemExpression item(const Item& value) {
    return ItemExpression(entity_expression(value, StaticString{"item"}, StaticString{"Item"}));
}
```

The [restricted SDK update](parity/generated_expression_identity_sdk_update_5_0_6.json.gz)
reproduces all 80 active SDKs. Exactly 188 canonical-root literals in 66 expression
headers change. Reversing only those arguments and formatting restores every
previous byte; all other functions, model/checker/query metadata and guidance
remain unchanged. Eighty provenance records retain the current producer hashes.
The [installed check](parity/generated_expression_identity_sdk_installed_5_0_6.json.gz)
verifies all 3,560 generated-file hashes after formatting. All 2,467 protected
original/historical artifacts remain unchanged. The earlier entry-qualification
helper recognizes this exact addition and validates both literal names before
removing it for historical body comparisons.

## Complete original and installed outcomes

The [complete before matrix](parity/generated_identity_expression_full_before_5_0_6.json.gz)
merges two terminal source/candidate runs with identical observers, inputs,
producer/runtime hashes and archived SDKs. Parent hashes and every actual Rust
build/run step are retained. It contains 100 root/message differences among 960
outcomes: 50 each in entity-override and combined configurations.

The [installed replay](parity/generated_expression_identity_installed_5_0_6.json.gz)
explicitly reuses those immutable original observations and compiles/runs five
fresh current C++ SDKs. All **960/960** outcomes agree. Each side contains 500
complete NotLoaded diagnostics, 330 value results, 10 legitimate NULL unwrap
errors and 120 JSON input errors. The latter preserve rejection of NULL for the
required platform relation; they are not accepted loaded entities. Every read
has zero SQL records. No descriptor, canonical name, path, ID or business value
is normalized.

The [result verifier](../tests/parity/verify_generated_identity_expression_results_5_0_6.py)
checks complete SDK/input/result cardinality, original registry/source lineage,
terminal stdout, all nine diagnostic fields, canonical roots, descriptor
identities, loaded/NULL/error coverage and zero implicit SQL. It validates the
original human text before the English adapter. Its
[ten effective corruptions](parity/generated_expression_identity_controls_5_0_6.json.gz)
all reject: missing SDK/result, boolean count, implicit SQL, wrong root, dropped
field, wrong native entry, altered original human text, added responsibility and
wrong NULL message.

## Controller corrections and final gates

The first Java runner attempt used the repository root and omitted test
resources. Six of 88 tests failed to find their model/corpus inputs. The corrected
runner uses the generator module directory and explicit test-resource classpath;
all [88 current Java tests pass](parity/generated_expression_identity_java_repaired_5_0_6.json.gz).
The [initial receipt](parity/generated_expression_identity_java_5_0_6.json.gz)
retains those input-resolution failures. No production correction was needed.

The first independent result-verifier invocation incorrectly expected the
rejected preparation state to be `json_full`. Actual original/native output
rejects `json_null` with `JSON_ENTITY_INPUT: invalid field type: platform_id`.
The verifier now asserts that exact observed state/error; the dataset and runtime
remain unchanged. It subsequently passes all 960 results and ten controls.

The [final installed gates](parity/generated_expression_identity_installed_gates_5_0_6.json.gz)
pass native 5/5, imported modules 3/3 and scoped ASan/UBSan 1/1. Runtime changes
are limited to canonical root construction and separate delegating/deleted
factory overloads. No layout member, diagnostic schema, allocation owner or
provider/query identity changes. EID01 is closed and G11 returns to qualified;
G13/G14/G17 and every remaining whole phase exit retain their own requirements. The historical complete native 877 run precedes this runtime
header change. No current full-suite, MIPS or physical-device execution is
claimed. Leak scanning remains disabled for the scoped sanitizer profile.

## Artifact lifecycle

The [cleanup receipt](parity/generated_expression_identity_cleanup_5_0_6.json.gz)
confirms removal of seven exact task-owned source/target/class/output directories
after their compiler and observer children exit. Successful measured runs removed
792170861 bytes; failed or unmeasured paths are separately recorded as absent,
without inventing a size. Stable native, sanitize and module preset outputs and
shared Maven/Cargo downloads remain available for reuse. No unrelated cleanup or
commit/push is part of this checkpoint.
