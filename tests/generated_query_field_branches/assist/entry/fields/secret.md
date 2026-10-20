# Field Assist: entry.secret

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `secret()` / `updateSecret(std::string_view)`; changed `changedSecret()`; evaluation `evalSecret()`; expression `getSecret()`.
- Scalar predicates: `withSecretIs`, `withSecretIsNot`, `withSecretGreaterThan`, `withSecretGreaterThanOrEqualTo`, `withSecretLessThan`, `withSecretLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withSecretBetween(lower, upper)`, `withSecretIn(std::span<const String>)`, `withSecretNotIn(std::span<const String>)`, `withSecretIsNull()`, `withSecretIsNotNull()`.
- Projection/order/group: `selectSecret()`, `orderBySecretAsc()`, `orderBySecretDesc()`, `groupBySecret()`.
- Group/count: `groupBySecret()`, `groupBySecretAs(alias)`, `groupBySecretWithFunction(alias, function)`, `countSecret()` and `countSecretAs(alias)`.
- Scalar aggregate: `sumSecret()` uses alias `"sum_secret"`; `sumSecretAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgSecret()` uses alias `"avg_secret"`; `avgSecretAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minSecret()` uses alias `"min_secret"`; `minSecretAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxSecret()` uses alias `"max_secret"`; `maxSecretAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withSecretContaining(text)`, `withSecretStartingWith(text)`, `withSecretEndingWith(text)`.

Null alias: `withSecretIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withSecretIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
