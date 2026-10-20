# Field Assist: intermediate_record.created_at

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `createdAt()` / `updateCreatedAt(const Timestamp&)`; changed `changedCreatedAt()`; evaluation `evalCreatedAt()`; expression `getCreatedAt()`.
- Scalar predicates: `withCreatedAtIs`, `withCreatedAtIsNot`, `withCreatedAtGreaterThan`, `withCreatedAtGreaterThanOrEqualTo`, `withCreatedAtLessThan`, `withCreatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withCreatedAtBetween(lower, upper)`, `withCreatedAtIn(std::span<const Timestamp>)`, `withCreatedAtNotIn(std::span<const Timestamp>)`, `withCreatedAtIsNull()`, `withCreatedAtIsNotNull()`.
- Projection/order/group: `selectCreatedAt()`, `orderByCreatedAtAsc()`, `orderByCreatedAtDesc()`, `groupByCreatedAt()`.
- Group/count: `groupByCreatedAt()`, `groupByCreatedAtAs(alias)`, `groupByCreatedAtWithFunction(alias, function)`, `countCreatedAt()` and `countCreatedAtAs(alias)`.
- Scalar aggregate: `sumCreatedAt()` uses alias `"sum_created_at"`; `sumCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCreatedAt()` uses alias `"avg_created_at"`; `avgCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCreatedAt()` uses alias `"min_created_at"`; `minCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCreatedAt()` uses alias `"max_created_at"`; `maxCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withCreatedAtIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCreatedAtIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
