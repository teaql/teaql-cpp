# Field Assist: note.date

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `date()` / `updateDate(const Date&)`; changed `changedDate()`; evaluation `evalDate()`; expression `getDate()`.
- Candidate filters: `withDateIsDateCreateFunctionNow()` and `withDateIsNotDateCreateFunctionNow()`; exact value `date(createFunction=now`.
- Candidate filters: `withDateIsUpdateFunctionNow()` and `withDateIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withDateIs`, `withDateIsNot`, `withDateGreaterThan`, `withDateGreaterThanOrEqualTo`, `withDateLessThan`, `withDateLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDateBetween(lower, upper)`, `withDateIn(std::span<const Date>)`, `withDateNotIn(std::span<const Date>)`, `withDateIsNull()`, `withDateIsNotNull()`.
- Projection/order/group: `selectDate()`, `orderByDateAsc()`, `orderByDateDesc()`, `groupByDate()`.
- Group/count: `groupByDate()`, `groupByDateAs(alias)`, `groupByDateWithFunction(alias, function)`, `countDate()` and `countDateAs(alias)`.
- Scalar aggregate: `sumDate()` uses alias `"sum_date"`; `sumDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDate()` uses alias `"avg_date"`; `avgDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDate()` uses alias `"min_date"`; `minDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDate()` uses alias `"max_date"`; `maxDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withDateIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withDateIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Date predicates: `withDateBefore(value)`, `withDateAfter(value)` and `withDateBetween(from, to)`.

[Model/action index](../../../ASSIST.md)
