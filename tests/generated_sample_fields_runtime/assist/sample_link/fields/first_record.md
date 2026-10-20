# Field Assist: sample_link.first_record

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `firstRecordId()` / `updateFirstRecordId(const std::uint64_t&)`; changed `changedFirstRecordId()`; evaluation `evalFirstRecordId()`; expression `getFirstRecordId()`.
- Scalar predicates: `withFirstRecordIs`, `withFirstRecordIsNot`, `withFirstRecordGreaterThan`, `withFirstRecordGreaterThanOrEqualTo`, `withFirstRecordLessThan`, `withFirstRecordLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withFirstRecordBetween(lower, upper)`, `withFirstRecordIn(std::span<const std::uint64_t>)`, `withFirstRecordNotIn(std::span<const std::uint64_t>)`, `withFirstRecordIsNull()`, `withFirstRecordIsNotNull()`.
- Projection/order/group: `selectFirstRecordId()`, `orderByFirstRecordIdAsc()`, `orderByFirstRecordIdDesc()`, `groupByFirstRecord()`.
- Group/count: `groupByFirstRecord()`, `groupByFirstRecordAs(alias)`, `groupByFirstRecordWithFunction(alias, function)`, `countFirstRecord()` and `countFirstRecordAs(alias)`.
- Forward relation: `firstRecord()` returns const SampleRecord*, `updateFirstRecord(const SampleRecord&)`; query `selectFirstRecord()`, `selectFirstRecordWith(SampleRecordRequest<P>)`, `withFirstRecord(SampleRecordRequest<P>)`; expression `getFirstRecord()`.

Null alias: `withFirstRecordIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withFirstRecordIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalFirstRecord()`. Matching: `withFirstRecordMatching(child)`, `withoutFirstRecordMatching(child)`. Presence: `haveFirstRecord()`, `haveNoFirstRecord()`. Facets: `facetByFirstRecordAs(name, child)`, `facetByFirstRecordAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByFirstRecordWith(child)`, `groupByFirstRecordWithDetails()`, `groupByFirstRecordWithDetailsFrom(child)`. Roll-up: `rollUpToFirstRecord()`, `rollUpToFirstRecordWith(child)`.

[Model/action index](../../../ASSIST.md)
