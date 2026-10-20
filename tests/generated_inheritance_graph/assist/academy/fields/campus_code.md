# Field Assist: academy.campus_code

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `campusCode()` / `updateCampusCode(std::string_view)`; changed `changedCampusCode()`; evaluation `evalCampusCode()`; expression `getCampusCode()`.
- Scalar predicates: `withCampusCodeIs`, `withCampusCodeIsNot`, `withCampusCodeGreaterThan`, `withCampusCodeGreaterThanOrEqualTo`, `withCampusCodeLessThan`, `withCampusCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withCampusCodeBetween(lower, upper)`, `withCampusCodeIn(std::span<const String>)`, `withCampusCodeNotIn(std::span<const String>)`, `withCampusCodeIsNull()`, `withCampusCodeIsNotNull()`.
- Projection/order/group: `selectCampusCode()`, `orderByCampusCodeAsc()`, `orderByCampusCodeDesc()`, `groupByCampusCode()`.
- Group/count: `groupByCampusCode()`, `groupByCampusCodeAs(alias)`, `groupByCampusCodeWithFunction(alias, function)`, `countCampusCode()` and `countCampusCodeAs(alias)`.
- Scalar aggregate: `sumCampusCode()` uses alias `"sum_campus_code"`; `sumCampusCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCampusCode()` uses alias `"avg_campus_code"`; `avgCampusCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCampusCode()` uses alias `"min_campus_code"`; `minCampusCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCampusCode()` uses alias `"max_campus_code"`; `maxCampusCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withCampusCodeContaining(text)`, `withCampusCodeStartingWith(text)`, `withCampusCodeEndingWith(text)`.

Null alias: `withCampusCodeIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCampusCodeIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withCampusCodeNotContaining(value)`.

Text predicate: `withCampusCodeNotStartingWith(value)`.

Text predicate: `withCampusCodeNotEndingWith(value)`.

Text predicate: `withCampusCodeSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
