# Field Assist: holder.record

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `recordId()` / `updateRecordId(const std::optional<std::uint64_t>&)`; changed `changedRecordId()`; evaluation `evalRecordId()`; expression `getRecordId()`.
- Scalar predicates: `withRecordIs`, `withRecordIsNot`, `withRecordGreaterThan`, `withRecordGreaterThanOrEqualTo`, `withRecordLessThan`, `withRecordLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withRecordBetween(lower, upper)`, `withRecordIn(std::span<const std::uint64_t>)`, `withRecordNotIn(std::span<const std::uint64_t>)`, `withRecordIsNull()`, `withRecordIsNotNull()`.
- Projection/order/group: `selectRecordId()`, `orderByRecordIdAsc()`, `orderByRecordIdDesc()`, `groupByRecord()`.
- Group/count: `groupByRecord()`, `groupByRecordAs(alias)`, `groupByRecordWithFunction(alias, function)`, `countRecord()` and `countRecordAs(alias)`.
- Forward relation: `record()` returns const Record*, `updateRecord(const Record&)`; query `selectRecord()`, `selectRecordWith(RecordRequest<P>)`, `withRecord(RecordRequest<P>)`; expression `getRecord()`.

Null alias: `withRecordIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withRecordIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalRecord()`. Matching: `withRecordMatching(child)`, `withoutRecordMatching(child)`. Presence: `haveRecord()`, `haveNoRecord()`. Facets: `facetByRecordAs(name, child)`, `facetByRecordAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByRecordWith(child)`, `groupByRecordWithDetails()`, `groupByRecordWithDetailsFrom(child)`. Roll-up: `rollUpToRecord()`, `rollUpToRecordWith(child)`.

[Model/action index](../../../ASSIST.md)
