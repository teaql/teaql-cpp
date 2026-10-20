# Field Assist: sample_record.negative_bound

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `negativeBound()` / `updateNegativeBound(std::string_view)`; changed `changedNegativeBound()`; evaluation `evalNegativeBound()`; expression `getNegativeBound()`.
- Scalar predicates: `withNegativeBoundIs`, `withNegativeBoundIsNot`, `withNegativeBoundGreaterThan`, `withNegativeBoundGreaterThanOrEqualTo`, `withNegativeBoundLessThan`, `withNegativeBoundLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNegativeBoundBetween(lower, upper)`, `withNegativeBoundIn(std::span<const String>)`, `withNegativeBoundNotIn(std::span<const String>)`, `withNegativeBoundIsNull()`, `withNegativeBoundIsNotNull()`.
- Projection/order/group: `selectNegativeBound()`, `orderByNegativeBoundAsc()`, `orderByNegativeBoundDesc()`, `groupByNegativeBound()`.
- Group/count: `groupByNegativeBound()`, `groupByNegativeBoundAs(alias)`, `groupByNegativeBoundWithFunction(alias, function)`, `countNegativeBound()` and `countNegativeBoundAs(alias)`.
- Scalar aggregate: `sumNegativeBound()` uses alias `"sum_negative_bound"`; `sumNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNegativeBound()` uses alias `"avg_negative_bound"`; `avgNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNegativeBound()` uses alias `"min_negative_bound"`; `minNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNegativeBound()` uses alias `"max_negative_bound"`; `maxNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNegativeBoundContaining(text)`, `withNegativeBoundStartingWith(text)`, `withNegativeBoundEndingWith(text)`.

Null alias: `withNegativeBoundIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withNegativeBoundIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withNegativeBoundNotContaining(value)`.

Text predicate: `withNegativeBoundNotStartingWith(value)`.

Text predicate: `withNegativeBoundNotEndingWith(value)`.

Text predicate: `withNegativeBoundSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
