# Field Assist: specialist.name

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `whoseNameIs`, `whoseNameIsNot`, `whoseNameGreaterThan`, `whoseNameGreaterThanOrEqualTo`, `whoseNameLessThan`, `whoseNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `whoseNameBetween(lower, upper)`, `whoseNameIn(std::span<const String>)`, `whoseNameNotIn(std::span<const String>)`, `whoseNameIsNull()`, `whoseNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `"sum_name"`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `"avg_name"`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `"min_name"`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `"max_name"`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `whoseNameContains(text)`, `whoseNameStartsWith(text)`, `whoseNameEndsWith(text)`.

Null alias: `whoseNameIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseNameIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `whoseNameNotContains(value)`.

Text predicate: `whoseNameNotStartsWith(value)`.

Text predicate: `whoseNameNotEndsWith(value)`.

Text predicate: `whoseNameSoundsLike(value)`.

[Model/action index](../../../ASSIST.md)
