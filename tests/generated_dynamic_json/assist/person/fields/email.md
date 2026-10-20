# Field Assist: person.email

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `email()` / `updateEmail(std::string_view)`; changed `changedEmail()`; evaluation `evalEmail()`; expression `getEmail()`.
- Scalar predicates: `whoseEmailIs`, `whoseEmailIsNot`, `whoseEmailGreaterThan`, `whoseEmailGreaterThanOrEqualTo`, `whoseEmailLessThan`, `whoseEmailLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `whoseEmailBetween(lower, upper)`, `whoseEmailIn(std::span<const String>)`, `whoseEmailNotIn(std::span<const String>)`, `whoseEmailIsNull()`, `whoseEmailIsNotNull()`.
- Projection/order/group: `selectEmail()`, `orderByEmailAsc()`, `orderByEmailDesc()`, `groupByEmail()`.
- Group/count: `groupByEmail()`, `groupByEmailAs(alias)`, `groupByEmailWithFunction(alias, function)`, `countEmail()` and `countEmailAs(alias)`.
- Scalar aggregate: `sumEmail()` uses alias `"sum_email"`; `sumEmailAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgEmail()` uses alias `"avg_email"`; `avgEmailAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minEmail()` uses alias `"min_email"`; `minEmailAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxEmail()` uses alias `"max_email"`; `maxEmailAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `whoseEmailContains(text)`, `whoseEmailStartsWith(text)`, `whoseEmailEndsWith(text)`.

Null alias: `whoseEmailIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseEmailIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `whoseEmailNotContains(value)`.

Text predicate: `whoseEmailNotStartsWith(value)`.

Text predicate: `whoseEmailNotEndsWith(value)`.

Text predicate: `whoseEmailSoundsLike(value)`.

[Model/action index](../../../ASSIST.md)
