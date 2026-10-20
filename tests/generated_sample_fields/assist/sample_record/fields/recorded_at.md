# Field Assist: sample_record.recorded_at

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `recordedAt()` / `updateRecordedAt(const Timestamp&)`; changed `changedRecordedAt()`; evaluation `evalRecordedAt()`; expression `getRecordedAt()`.
- Scalar predicates: `withRecordedAtIs`, `withRecordedAtIsNot`, `withRecordedAtGreaterThan`, `withRecordedAtGreaterThanOrEqualTo`, `withRecordedAtLessThan`, `withRecordedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withRecordedAtBetween(lower, upper)`, `withRecordedAtIn(std::span<const Timestamp>)`, `withRecordedAtNotIn(std::span<const Timestamp>)`, `withRecordedAtIsNull()`, `withRecordedAtIsNotNull()`.
- Projection/order/group: `selectRecordedAt()`, `orderByRecordedAtAsc()`, `orderByRecordedAtDesc()`, `groupByRecordedAt()`.
- Group/count: `groupByRecordedAt()`, `groupByRecordedAtAs(alias)`, `groupByRecordedAtWithFunction(alias, function)`, `countRecordedAt()` and `countRecordedAtAs(alias)`.
- Scalar aggregate: `sumRecordedAt()` uses alias `"sum_recorded_at"`; `sumRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRecordedAt()` uses alias `"avg_recorded_at"`; `avgRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRecordedAt()` uses alias `"min_recorded_at"`; `minRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRecordedAt()` uses alias `"max_recorded_at"`; `maxRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withRecordedAtIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withRecordedAtIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
