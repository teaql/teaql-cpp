# Callable scalar keyword diagnostic repairs

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The implementation remains in the independent `cpp_support` workspace.
This checkpoint extends G11/G14 evidence; it does not complete G14 or P0–P7.

## Reproduced gap and implementation

For a scalar named `class`, existing generated E access uses the canonical
`class` diagnostic path. The runtime correctly suggests `selectClass()`, but
before this change the C++ request only provided `selectModelClass()`.
The real compiler rejection and complete pre-change SDKs are preserved in
[the missing-method record](parity/scalar_diagnostic_missing_api_5_0_6.json.gz)
and [the 76-SDK snapshot](parity/scalar_diagnostic_sdks_before_fix_5_0_6.json.gz).

The generator now emits three canonical aliases for scalar fields whose storage
identity is a C++ keyword:

```cpp
request.selectClass();
request.unselectClass();
auto expression = E::record(record).getClass();
```

The Q methods forward to the existing escaped selection methods, with the same
lvalue/rvalue contracts. The E method returns the existing escaped getter's
expression. Original methods remain available. Metadata, SQL identifiers,
projections, load state and the runtime diagnostic renderer are unchanged.
The new per-field Assist paragraph documents both method spellings and their
canonical diagnostic path.

These aliases introduce no execution, allocation or ownership algorithm. They
use the original Context binding and exactly one execution UserContext.
They neither perform implicit SQL nor extend either Context lifetime.
The affected retained models exercise `class`, `operator`, `template`, `long`,
`float` and `double`. This finite checkpoint does not qualify arbitrary renamed
storage fields or every reserved/Rust-keyword/inherited configuration.

## Actual original SQLite observations

The new isolated Rust observer uses the complete unchanged frozen generated SDK,
its actual generated runtime, the original SQLite provider and the retained
`query_alias_sql.sql` data. Registry versions and checksums must belong to the
locked original Cargo dependency graph. The observer does not implement query,
decode or diagnostic algorithms.

Its immutable 480-case corpus crosses:

- Three scalar fields: `class`, `operator` and `template`.
- Empty, selected, removed, reselected and self projections.
- Ordinary/minimal queries, active/deleted-row modes and active/deleted row IDs.
- E `eval`/`unwrap` and existing/canonical C++ method spellings.

The original provides 72 values, 72 complete missing-load diagnostics, 48 absent
rows and 288 errors. The ordinary/self projection's existing `type` field error
is preserved. Neither observer converts that failure into a successful load.
Legacy and canonical C++ spellings are compared separately with the same actual
original APIs.

Comparison retains complete SelectQuery Debug, presence, scalar/optional Debug,
error text and all 192 ordered SQL diagnostics with bindings, comment, purpose and trace.
The full diagnostic object is compared using the already documented C++ syntax
and English-message adapter from
[expression diagnostics](generated_expression_diagnostics_5_0_6.md).
No new result, SQL, error, identity or privacy normalization is introduced.

## Ownership and exact generated-body checks

The native contract exercises a query returned from a helper, both fluent value
categories, explicit destination copying and rejection of execution with a
foreign Context. Integer, float and double E return types are checked, together
with their actual values and the continued use of escaped getters.

After actual source UserContext deletion, both the entity-expression and scalar
expression wrappers reject access. An explicit Global destination copy remains
readable through the destination UserContext. Default PMR allocation is disabled.
The SQLite comparison also deletes each UserContext and requires zero retained
User bytes.

The metadata probe obtains fields, C++ keyword classification, types and method
spellings from the actual parser/producer. The SDK verifier removes only the
exact corresponding Q/E delegates and checks every preceding byte after pinned
formatting; only precise per-field Assist lines may be appended. All 76 complete
SDK file sets and recorded hashes remain checked. Earlier relationship and ledger
proofs replay after separately removing these qualified later additions; their
immutable snapshots and historical reports are not rewritten.

The 63 locked source field-method bodies, query constructor and eight other
reviewed native helper bodies remain unchanged. Only the reviewed Assist helper
changes in that existing nine-helper index. The new review record is separate
from the previous checkpoint.

## Verified checks

| Check | Result |
| --- | --- |
| Actual original/native SQLite comparison | 480/480, using only the existing diagnostic presentation adapter |
| Native selected regression | 5/5, including retained relationship, aggregate-alias and E diagnostic checks |
| Imported-module comparison and ownership contracts | 2/2 |
| Application compiler controls | 1 positive and 4 negative |
| Producer Java checks | 68 passed |
| Complete generated SDK change proof | 76 SDKs; only two have new code/Assist |
| Strict generation reproduction | 24 normal workspaces, 50 contract fixtures, two keyword SDK pairs and the alias entry point |
| Historical body-proof replay | Ledger composition across 76 SDKs; relationship identity across 24 normal SDKs |
| Canonical module consumer bodies | 769 verified |
| Pinned C++ formatting and check | 1,610 inputs |

Final strict regeneration, formatting and historical proof replay are retained in
[the terminal evidence](parity/scalar_diagnostic_terminal_5_0_6.json.gz).
No full-suite, sanitizer, MIPS, physical-router or external integration result is
claimed for this adapter change.

## Reproduction

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_scalar_diagnostic_sdk_update_5_0_6.py
cmake --build --preset native --target \
  teaql_scalar_diagnostic_tests teaql_scalar_diagnostic_repair_probe
ctest --preset native --output-on-failure \
  -R '^(generated_scalar_diagnostic_contracts|scalar_diagnostic_repair_rust_parity)$'
```

The imported-module frontend consumes the same canonical test and observer bodies
through `teaql_provider_sqlite`. Generation remains a development operation;
no new production dependency or runtime kernel change is introduced.
