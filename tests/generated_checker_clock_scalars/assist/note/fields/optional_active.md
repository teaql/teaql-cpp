# Field Assist: note.optional_active

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalActive()` / `updateOptionalActive(const std::optional<bool>&)`; changed `changedOptionalActive()`; evaluation `evalOptionalActive()`; expression `getOptionalActive()`.
- Candidate filters: `withOptionalActiveIsBoolCreateFunctionNow()` and `withOptionalActiveIsNotBoolCreateFunctionNow()`; exact value `bool(createFunction=now`.
- Candidate filters: `withOptionalActiveIsUpdateFunctionNow()` and `withOptionalActiveIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalActiveIs`, `withOptionalActiveIsNot`, `withOptionalActiveGreaterThan`, `withOptionalActiveGreaterThanOrEqualTo`, `withOptionalActiveLessThan`, `withOptionalActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withOptionalActiveBetween(lower, upper)`, `withOptionalActiveIn(std::span<const bool>)`, `withOptionalActiveNotIn(std::span<const bool>)`, `withOptionalActiveIsNull()`, `withOptionalActiveIsNotNull()`.
- Projection/order/group: `selectOptionalActive()`, `orderByOptionalActiveAsc()`, `orderByOptionalActiveDesc()`, `groupByOptionalActive()`.
- Group/count: `groupByOptionalActive()`, `groupByOptionalActiveAs(alias)`, `groupByOptionalActiveWithFunction(alias, function)`, `countOptionalActive()` and `countOptionalActiveAs(alias)`.
- Scalar aggregate: `sumOptionalActive()` uses alias `"sum_optional_active"`; `sumOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalActive()` uses alias `"avg_optional_active"`; `avgOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalActive()` uses alias `"min_optional_active"`; `minOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalActive()` uses alias `"max_optional_active"`; `maxOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreOptionalActive()`, `whichAreNotOptionalActive()`.

Null alias: `withOptionalActiveIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalActiveIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
