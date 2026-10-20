# Field Assist: person.birthday

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `birthday()` / `updateBirthday(const Date&)`; changed `changedBirthday()`; evaluation `evalBirthday()`; expression `getBirthday()`.
- Candidate filters: `whoseBirthdayIsDate()` and `whoseBirthdayIsNotDate()`; exact value `date()`.
- Scalar predicates: `whoseBirthdayIs`, `whoseBirthdayIsNot`, `whoseBirthdayGreaterThan`, `whoseBirthdayGreaterThanOrEqualTo`, `whoseBirthdayLessThan`, `whoseBirthdayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `whoseBirthdayBetween(lower, upper)`, `whoseBirthdayIn(std::span<const Date>)`, `whoseBirthdayNotIn(std::span<const Date>)`, `whoseBirthdayIsNull()`, `whoseBirthdayIsNotNull()`.
- Projection/order/group: `selectBirthday()`, `orderByBirthdayAsc()`, `orderByBirthdayDesc()`, `groupByBirthday()`.
- Group/count: `groupByBirthday()`, `groupByBirthdayAs(alias)`, `groupByBirthdayWithFunction(alias, function)`, `countBirthday()` and `countBirthdayAs(alias)`.
- Scalar aggregate: `sumBirthday()` uses alias `"sum_birthday"`; `sumBirthdayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgBirthday()` uses alias `"avg_birthday"`; `avgBirthdayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minBirthday()` uses alias `"min_birthday"`; `minBirthdayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxBirthday()` uses alias `"max_birthday"`; `maxBirthdayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `whoseBirthdayIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseBirthdayIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Date predicates: `whoseBirthdayBefore(value)`, `whoseBirthdayAfter(value)` and `whoseBirthdayBetween(from, to)`.

[Model/action index](../../../ASSIST.md)
