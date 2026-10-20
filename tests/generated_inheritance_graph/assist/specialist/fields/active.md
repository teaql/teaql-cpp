# Field Assist: specialist.active

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `whoseActiveIs`, `whoseActiveIsNot`, `whoseActiveGreaterThan`, `whoseActiveGreaterThanOrEqualTo`, `whoseActiveLessThan`, `whoseActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `whoseActiveBetween(lower, upper)`, `whoseActiveIn(std::span<const bool>)`, `whoseActiveNotIn(std::span<const bool>)`, `whoseActiveIsNull()`, `whoseActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `"sum_active"`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `"avg_active"`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `"min_active"`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `"max_active"`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whoAreActive()`, `whoAreNotActive()`.

Null alias: `whoseActiveIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseActiveIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
