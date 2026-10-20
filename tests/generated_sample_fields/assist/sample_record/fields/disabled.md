# Field Assist: sample_record.disabled

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `disabled()` / `updateDisabled(const bool&)`; changed `changedDisabled()`; evaluation `evalDisabled()`; expression `getDisabled()`.
- Scalar predicates: `withDisabledIs`, `withDisabledIsNot`, `withDisabledGreaterThan`, `withDisabledGreaterThanOrEqualTo`, `withDisabledLessThan`, `withDisabledLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withDisabledBetween(lower, upper)`, `withDisabledIn(std::span<const bool>)`, `withDisabledNotIn(std::span<const bool>)`, `withDisabledIsNull()`, `withDisabledIsNotNull()`.
- Projection/order/group: `selectDisabled()`, `orderByDisabledAsc()`, `orderByDisabledDesc()`, `groupByDisabled()`.
- Group/count: `groupByDisabled()`, `groupByDisabledAs(alias)`, `groupByDisabledWithFunction(alias, function)`, `countDisabled()` and `countDisabledAs(alias)`.
- Scalar aggregate: `sumDisabled()` uses alias `"sum_disabled"`; `sumDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDisabled()` uses alias `"avg_disabled"`; `avgDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDisabled()` uses alias `"min_disabled"`; `minDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDisabled()` uses alias `"max_disabled"`; `maxDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreDisabled()`, `whichAreNotDisabled()`.

Null alias: `withDisabledIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withDisabledIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
