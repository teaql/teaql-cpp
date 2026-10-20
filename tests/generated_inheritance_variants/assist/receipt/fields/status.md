# Field Assist: receipt.status

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `status()` / `updateStatus(std::string_view)`; changed `changedStatus()`; evaluation `evalStatus()`; expression `getStatus()`.
- Scalar predicates: `withStatusIs`, `withStatusIsNot`, `withStatusGreaterThan`, `withStatusGreaterThanOrEqualTo`, `withStatusLessThan`, `withStatusLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withStatusBetween(lower, upper)`, `withStatusIn(std::span<const String>)`, `withStatusNotIn(std::span<const String>)`, `withStatusIsNull()`, `withStatusIsNotNull()`.
- Projection/order/group: `selectStatus()`, `orderByStatusAsc()`, `orderByStatusDesc()`, `groupByStatus()`.
- Group/count: `groupByStatus()`, `groupByStatusAs(alias)`, `groupByStatusWithFunction(alias, function)`, `countStatus()` and `countStatusAs(alias)`.
- Scalar aggregate: `sumStatus()` uses alias `"sum_status"`; `sumStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgStatus()` uses alias `"avg_status"`; `avgStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minStatus()` uses alias `"min_status"`; `minStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxStatus()` uses alias `"max_status"`; `maxStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withStatusContaining(text)`, `withStatusStartingWith(text)`, `withStatusEndingWith(text)`.

Null alias: `withStatusIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withStatusIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withStatusNotContaining(value)`.

Text predicate: `withStatusNotStartingWith(value)`.

Text predicate: `withStatusNotEndingWith(value)`.

Text predicate: `withStatusSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
