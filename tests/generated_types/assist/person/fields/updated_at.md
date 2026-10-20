# Field Assist: person.updated_at

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `updatedAt()` / `updateUpdatedAt(const Timestamp&)`; changed `changedUpdatedAt()`; evaluation `evalUpdatedAt()`; expression `getUpdatedAt()`.
- Candidate filters: `whoseUpdatedAtIsDateTime()` and `whoseUpdatedAtIsNotDateTime()`; exact value `dateTime()`.
- Scalar predicates: `whoseUpdatedAtIs`, `whoseUpdatedAtIsNot`, `whoseUpdatedAtGreaterThan`, `whoseUpdatedAtGreaterThanOrEqualTo`, `whoseUpdatedAtLessThan`, `whoseUpdatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `whoseUpdatedAtBetween(lower, upper)`, `whoseUpdatedAtIn(std::span<const Timestamp>)`, `whoseUpdatedAtNotIn(std::span<const Timestamp>)`, `whoseUpdatedAtIsNull()`, `whoseUpdatedAtIsNotNull()`.
- Projection/order/group: `selectUpdatedAt()`, `orderByUpdatedAtAsc()`, `orderByUpdatedAtDesc()`, `groupByUpdatedAt()`.
- Group/count: `groupByUpdatedAt()`, `groupByUpdatedAtAs(alias)`, `groupByUpdatedAtWithFunction(alias, function)`, `countUpdatedAt()` and `countUpdatedAtAs(alias)`.
- Scalar aggregate: `sumUpdatedAt()` uses alias `"sum_updated_at"`; `sumUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgUpdatedAt()` uses alias `"avg_updated_at"`; `avgUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minUpdatedAt()` uses alias `"min_updated_at"`; `minUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxUpdatedAt()` uses alias `"max_updated_at"`; `maxUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `whoseUpdatedAtIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseUpdatedAtIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
