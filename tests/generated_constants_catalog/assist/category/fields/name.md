# Field Assist: category.name

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Candidate filters: `withNameIsFirstCategory()` and `withNameIsNotFirstCategory()`; exact value `First category`.
- Candidate filters: `withNameIsSecondCategory()` and `withNameIsNotSecondCategory()`; exact value `Second category`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `"sum_name"`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `"avg_name"`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `"min_name"`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `"max_name"`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.

Null alias: `withNameIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withNameIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withNameNotContaining(value)`.

Text predicate: `withNameNotStartingWith(value)`.

Text predicate: `withNameNotEndingWith(value)`.

Text predicate: `withNameSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
