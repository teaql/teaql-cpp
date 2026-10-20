# Field Assist: sample_record.optional_recorded_at

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalRecordedAt()` / `updateOptionalRecordedAt(const std::optional<Timestamp>&)`; changed `changedOptionalRecordedAt()`; evaluation `evalOptionalRecordedAt()`; expression `getOptionalRecordedAt()`.
- Scalar predicates: `withOptionalRecordedAtIs`, `withOptionalRecordedAtIsNot`, `withOptionalRecordedAtGreaterThan`, `withOptionalRecordedAtGreaterThanOrEqualTo`, `withOptionalRecordedAtLessThan`, `withOptionalRecordedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withOptionalRecordedAtBetween(lower, upper)`, `withOptionalRecordedAtIn(std::span<const Timestamp>)`, `withOptionalRecordedAtNotIn(std::span<const Timestamp>)`, `withOptionalRecordedAtIsNull()`, `withOptionalRecordedAtIsNotNull()`.
- Projection/order/group: `selectOptionalRecordedAt()`, `orderByOptionalRecordedAtAsc()`, `orderByOptionalRecordedAtDesc()`, `groupByOptionalRecordedAt()`.
- Group/count: `groupByOptionalRecordedAt()`, `groupByOptionalRecordedAtAs(alias)`, `groupByOptionalRecordedAtWithFunction(alias, function)`, `countOptionalRecordedAt()` and `countOptionalRecordedAtAs(alias)`.
- Scalar aggregate: `sumOptionalRecordedAt()` uses alias `"sum_optional_recorded_at"`; `sumOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalRecordedAt()` uses alias `"avg_optional_recorded_at"`; `avgOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalRecordedAt()` uses alias `"min_optional_recorded_at"`; `minOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalRecordedAt()` uses alias `"max_optional_recorded_at"`; `maxOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withOptionalRecordedAtIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalRecordedAtIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
