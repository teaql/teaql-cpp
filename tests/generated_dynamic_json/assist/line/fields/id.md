# Field Assist: line.id

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `"sum_id"`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `"avg_id"`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `"min_id"`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `"max_id"`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withIdIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withIdIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
