# Field Assist: sample_link.second_record

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `secondRecordId()` / `updateSecondRecordId(const std::uint64_t&)`; changed `changedSecondRecordId()`; evaluation `evalSecondRecordId()`; expression `getSecondRecordId()`.
- Scalar predicates: `withSecondRecordIs`, `withSecondRecordIsNot`, `withSecondRecordGreaterThan`, `withSecondRecordGreaterThanOrEqualTo`, `withSecondRecordLessThan`, `withSecondRecordLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withSecondRecordBetween(lower, upper)`, `withSecondRecordIn(std::span<const std::uint64_t>)`, `withSecondRecordNotIn(std::span<const std::uint64_t>)`, `withSecondRecordIsNull()`, `withSecondRecordIsNotNull()`.
- Projection/order/group: `selectSecondRecordId()`, `orderBySecondRecordIdAsc()`, `orderBySecondRecordIdDesc()`, `groupBySecondRecord()`.
- Group/count: `groupBySecondRecord()`, `groupBySecondRecordAs(alias)`, `groupBySecondRecordWithFunction(alias, function)`, `countSecondRecord()` and `countSecondRecordAs(alias)`.
- Forward relation: `secondRecord()` returns const SampleRecord*, `updateSecondRecord(const SampleRecord&)`; query `selectSecondRecord()`, `selectSecondRecordWith(SampleRecordRequest<P>)`, `withSecondRecord(SampleRecordRequest<P>)`; expression `getSecondRecord()`.

Null alias: `withSecondRecordIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withSecondRecordIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalSecondRecord()`. Matching: `withSecondRecordMatching(child)`, `withoutSecondRecordMatching(child)`. Presence: `haveSecondRecord()`, `haveNoSecondRecord()`. Facets: `facetBySecondRecordAs(name, child)`, `facetBySecondRecordAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupBySecondRecordWith(child)`, `groupBySecondRecordWithDetails()`, `groupBySecondRecordWithDetailsFrom(child)`. Roll-up: `rollUpToSecondRecord()`, `rollUpToSecondRecordWith(child)`.

[Model/action index](../../../ASSIST.md)
