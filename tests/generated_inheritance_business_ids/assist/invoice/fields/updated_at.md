# Field Assist: invoice.updated_at

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `updatedAt()` / `updateUpdatedAt(const Timestamp&)`; changed `changedUpdatedAt()`; evaluation `evalUpdatedAt()`; expression `getUpdatedAt()`.
- Scalar predicates: `withUpdatedAtIs`, `withUpdatedAtIsNot`, `withUpdatedAtGreaterThan`, `withUpdatedAtGreaterThanOrEqualTo`, `withUpdatedAtLessThan`, `withUpdatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withUpdatedAtBetween(lower, upper)`, `withUpdatedAtIn(std::span<const Timestamp>)`, `withUpdatedAtNotIn(std::span<const Timestamp>)`, `withUpdatedAtIsNull()`, `withUpdatedAtIsNotNull()`.
- Projection/order/group: `selectUpdatedAt()`, `orderByUpdatedAtAsc()`, `orderByUpdatedAtDesc()`, `groupByUpdatedAt()`.
- Group/count: `groupByUpdatedAt()`, `groupByUpdatedAtAs(alias)`, `groupByUpdatedAtWithFunction(alias, function)`, `countUpdatedAt()` and `countUpdatedAtAs(alias)`.
- Scalar aggregate: `sumUpdatedAt()` uses alias `"sum_updated_at"`; `sumUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgUpdatedAt()` uses alias `"avg_updated_at"`; `avgUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minUpdatedAt()` uses alias `"min_updated_at"`; `minUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxUpdatedAt()` uses alias `"max_updated_at"`; `maxUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withUpdatedAtIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withUpdatedAtIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
