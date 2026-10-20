# Field Assist: person.age

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `age()` / `updateAge(const std::int64_t&)`; changed `changedAge()`; evaluation `evalAge()`; expression `getAge()`.
- Candidate filters: `whoseAgeIsInteger()` and `whoseAgeIsNotInteger()`; exact value `integer()`.
- Scalar predicates: `whoseAgeIs`, `whoseAgeIsNot`, `whoseAgeGreaterThan`, `whoseAgeGreaterThanOrEqualTo`, `whoseAgeLessThan`, `whoseAgeLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `whoseAgeBetween(lower, upper)`, `whoseAgeIn(std::span<const std::int64_t>)`, `whoseAgeNotIn(std::span<const std::int64_t>)`, `whoseAgeIsNull()`, `whoseAgeIsNotNull()`.
- Projection/order/group: `selectAge()`, `orderByAgeAsc()`, `orderByAgeDesc()`, `groupByAge()`.
- Group/count: `groupByAge()`, `groupByAgeAs(alias)`, `groupByAgeWithFunction(alias, function)`, `countAge()` and `countAgeAs(alias)`.
- Scalar aggregate: `sumAge()` uses alias `"sum_age"`; `sumAgeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgAge()` uses alias `"avg_age"`; `avgAgeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minAge()` uses alias `"min_age"`; `minAgeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxAge()` uses alias `"max_age"`; `maxAgeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationAge()` and `standardDeviationAgeAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationAge()` and `squareRootOfPopulationStandardDeviationAgeAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceAge()` and `sampleVarianceAgeAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceAge()` and `samplePopulationVarianceAgeAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `whoseAgeIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseAgeIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
