# Field Assist: specialist.version

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `whoseVersionIs`, `whoseVersionIsNot`, `whoseVersionGreaterThan`, `whoseVersionGreaterThanOrEqualTo`, `whoseVersionLessThan`, `whoseVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `whoseVersionBetween(lower, upper)`, `whoseVersionIn(std::span<const std::int64_t>)`, `whoseVersionNotIn(std::span<const std::int64_t>)`, `whoseVersionIsNull()`, `whoseVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `"sum_version"`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `"avg_version"`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `"min_version"`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `"max_version"`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `whoseVersionIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseVersionIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
