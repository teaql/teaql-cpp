# Field Assist: record.template

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `modelTemplate()` / `updateModelTemplate(const Decimal&)`; changed `changedModelTemplate()`; evaluation `evalModelTemplate()`; expression `getModelTemplate()`.
- Scalar predicates: `withTemplateIs`, `withTemplateIsNot`, `withTemplateGreaterThan`, `withTemplateGreaterThanOrEqualTo`, `withTemplateLessThan`, `withTemplateLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withTemplateBetween(lower, upper)`, `withTemplateIn(std::span<const Decimal>)`, `withTemplateNotIn(std::span<const Decimal>)`, `withTemplateIsNull()`, `withTemplateIsNotNull()`.
- Projection/order/group: `selectModelTemplate()`, `orderByModelTemplateAsc()`, `orderByModelTemplateDesc()`, `groupByModelTemplate()`.
- Group/count: `groupByModelTemplate()`, `groupByModelTemplateAs(alias)`, `groupByModelTemplateWithFunction(alias, function)`, `countModelTemplate()` and `countModelTemplateAs(alias)`.
- Scalar aggregate: `sumModelTemplate()` uses alias `"sum_template"`; `sumModelTemplateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelTemplate()` uses alias `"avg_template"`; `avgModelTemplateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelTemplate()` uses alias `"min_template"`; `minModelTemplateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelTemplate()` uses alias `"max_template"`; `maxModelTemplateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelTemplate()` and `standardDeviationModelTemplateAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelTemplate()` and `squareRootOfPopulationStandardDeviationModelTemplateAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelTemplate()` and `sampleVarianceModelTemplateAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelTemplate()` and `samplePopulationVarianceModelTemplateAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withTemplateIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTemplateIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
