# Typed Business-ID query factories

This implements the GQ01 omission identified by the
[complete general-query review](general_query_requirements_5_0_6.md).
The reference remains Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The generated Context-bound factory exposes:

```cpp
auto Q = user_context.queries<cpp_business_ids::query_factory>();
auto number = cpp_business_ids::ReceiptNumber::create(
    user_context.memory_resource(), "RCP-example");
if (!number) {
    throw std::runtime_error(std::string(number.error()));
}
auto request = Q.receiptByReceiptNumber(number.value());
```

The retained models add `receiptByReceiptNumber`, `ticketByTicketNumber` and
`documentByDocumentNumber`. Each takes its distinct identifier type and delegates
to the normal Q entry, its existing typed equality predicate and `limit(2)`.
It adds no Context argument, identifier allocation, SQL, policy decision or
alternate query/ownership implementation. Callers can continue composing the
request. The limit can be overridden, and executeForOne retains the original
first-or-empty contract; the factory does not assert uniqueness.

## Eligibility and naming

The original `qMethod` invokes `rustBusinessIdLookup` for an entity's **own**
Business-ID fields. Inherited accessors do not create descendant-owned factory
methods. The C++ producer uses its existing Business-ID metadata adapter over
own fields: the older workspace model has no original getBusinessIdFields
getter, while its locked metadata adapter already supplies that classification.
No other-language model/template change is required.

Entity/field composition and lowerCamelCase use the existing identifier and
central naming services. Normal entries use centralized pluralization; typed
predicates use the existing human/non-human naming service. The Java regression
checks explicit human classification, a non-human Person, inherited omission,
C++ keyword fields and multiple identifier fields. The human test fixture uses
the parser's `_cat` metadata syntax; an earlier `cat` fixture was corrected
instead of changing the naming algorithm.

Model-specific Assist records the actual method/type/delegate and explains the
limit and execution boundary. Generated files are rendered by the producer;
they are not patched by hand.

## Runtime-first source comparison

The existing shared mechanics were qualified before factory emission. The
observer compares original Q lookup functions against ordinary typed predicate
composition. All 528 outcomes match. Cases cross Receipt/Ticket/Document,
ordinary and Unicode whitespace, empty identifiers, quotes, Unicode, embedded
NUL and long values, plus unchanged/overridden/offset/deleted-row composition.
Receipt and Ticket also execute compact rows, typed lists, first-or-empty and
counts against retained rows with duplicate numbers, a deleted row and another
tenant. This is an existing-database query proof, not schema/bootstrap,
identifier-profile validation or authorization acceptance.

The identifier constructor trims and rejects blank text. It does not validate
an allocation profile; that remains the BusinessIdService responsibility.
Normal query policy and Context assembly still govern execution. A lookup
factory does not automatically add application-owned tenant authorization.

For the fixture's `match` identifier, the normal request has four live matches,
including the other tenant's row: list returns two, count returns four and one
returns the first. Raising the limit to ten returns four; including deleted rows
returns five. These observations keep slicing, first-match semantics and
application authorization as separate responsibilities.

Document observations use the previously qualified inherited model-binding
bridge from G04. The observer requires its `src/q.rs` to be byte-identical to the
complete unchanged original crate. Lookup algorithms are not patched. The
original artifact's separate binding failures and their explicit adaptations
are preserved; this does not relabel the untouched crate as compiling.

The output contract retains identifier values/errors, complete lookup/manual/
post-composition query Debug, decoded JSON/availability or compact values,
scalar results, full errors and timeless safe SQL records. No declared field is
normalized. Native observers disable default PMR and close each actual Context.
The later default probe mode requires generated factories and throws if absent;
`--runtime-composition` is a distinct preliminary gate, not a silent fallback.

## Body preservation and ownership

The [complete SDK before snapshot](parity/business_id_lookup_sdks_before_5_0_6.json.gz)
and [producer before snapshot](parity/business_id_lookup_producer_before_5_0_6.json.gz)
preserve the migration boundary. The SDK verifier parses actual models, uses
the established naming/metadata implementation and permits only exact typed
lookup delegates and their matching Assist lines. It rejects unrelated changes.
The old source reviews remain historical; new reviews capture the current
producer helpers without relabeling their earlier hashes.

The ownership test destroys a temporary identifier wrapper before inspecting
the request, verifies independent explicit destination copies, closes the
source UserContext, rejects stale request/factory access and rebinds an explicit
Global copy. Neither identifier wrappers nor factories extend Context lifetime.

## Completed verification

| Gate | Result |
| --- | --- |
| Runtime-first and regenerated factory comparisons | 528/528 each; later probe requires the generated factories |
| Successful SQLite operations in each comparison | 288, with 288 complete safe SQL records |
| Identifier errors / successful construction-only cases | 132 / 108 |
| Relevant native regression | 6/6 |
| Imported-module checks | 2/2 |
| Targeted ASan/UBSan | 2/2; leak scanning disabled |
| Application compiler contracts | 2 positive / 5 negative |
| Java generation/layout/JSON checks | 71/71 |
| Strict ordinary generation | 24 workspaces / 288 files |
| Strict contract generation | 50 cases, including retained source-invalid models |
| Identity and expression-root reproduction | Both identity pairs and the original/native root pair |
| Whole-SDK scope proof | 77 SDKs; three delegates in two SDKs; every old body unchanged |
| Earlier migration proofs | Result-marker and E-entry SDK checks pass |
| Formatting / canonical module consumers / exports | 1,633 files / 784 bodies / 1,495 names |

The first ownership-test compilation mistakenly passed a string-error identifier
Result to the framework-error `require_value` helper. The corrected test checks
the Result before using its value; production code was not changed to accommodate
it. The Java fixture and metadata-probe preparation failures are retained with
their terminal successful follow-ups. A source-invalid model has no emitted
query header and is checked through its unchanged failure record, rather than
being treated as a successful model parse.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
    teaql_business_id_lookups_probe teaql_business_id_lookup_tests
ctest --preset native --output-on-failure \
    -R '^(business_id_lookups_rust_parity|business_id_lookup_context_contracts)$'
python3 scripts/verify_business_id_lookup_sdk_update_5_0_6.py
python3 scripts/verify_general_query_source_5_0_6.py
```

Current command results and source identities are retained in the
[terminal record](parity/business_id_lookups_terminal_5_0_6.json.gz).
This named repair does not by itself complete G06, P4/P7 or the full generator
goal. GPU/external providers, physical-device execution and full-suite acceptance
are separate claims.
