# Field Assist: note.optional_date

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalDate()` / `updateOptionalDate(const std::optional<Date>&)`; changed `changedOptionalDate()`; evaluation `evalOptionalDate()`; expression `getOptionalDate()`.
- Candidate filters: `withOptionalDateIsDateCreateFunctionNow()` and `withOptionalDateIsNotDateCreateFunctionNow()`; exact value `date(createFunction=now`.
- Candidate filters: `withOptionalDateIsUpdateFunctionNow()` and `withOptionalDateIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalDateIs`, `withOptionalDateIsNot`, `withOptionalDateGreaterThan`, `withOptionalDateGreaterThanOrEqualTo`, `withOptionalDateLessThan`, `withOptionalDateLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withOptionalDateBetween(lower, upper)`, `withOptionalDateIn(std::span<const Date>)`, `withOptionalDateNotIn(std::span<const Date>)`, `withOptionalDateIsNull()`, `withOptionalDateIsNotNull()`.
- Projection/order/group: `selectOptionalDate()`, `orderByOptionalDateAsc()`, `orderByOptionalDateDesc()`, `groupByOptionalDate()`.
- Group/count: `groupByOptionalDate()`, `groupByOptionalDateAs(alias)`, `groupByOptionalDateWithFunction(alias, function)`, `countOptionalDate()` and `countOptionalDateAs(alias)`.
- Scalar aggregate: `sumOptionalDate()` uses alias `"sum_optional_date"`; `sumOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDate()` uses alias `"avg_optional_date"`; `avgOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDate()` uses alias `"min_optional_date"`; `minOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDate()` uses alias `"max_optional_date"`; `maxOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withOptionalDateIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalDateIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Date predicates: `withOptionalDateBefore(value)`, `withOptionalDateAfter(value)` and `withOptionalDateBetween(from, to)`.

[Model/action index](../../../ASSIST.md)
