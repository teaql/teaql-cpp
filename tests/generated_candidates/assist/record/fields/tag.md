# Field Assist: record.tag

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `tag()` / `updateTag(const std::optional<String>&)`; changed `changedTag()`; evaluation `evalTag()`; expression `getTag()`.
- Candidate filters: `withTagIsRed()` and `withTagIsNotRed()`; exact value `red`.
- Candidate filters: `withTagIsBlue()` and `withTagIsNotBlue()`; exact value `blue`.
- Candidate filters: `withTagIsNull()` and `withTagIsNotNull()`; exact value `null`.
- Scalar predicates: `withTagIs`, `withTagIsNot`, `withTagGreaterThan`, `withTagGreaterThanOrEqualTo`, `withTagLessThan`, `withTagLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withTagBetween(lower, upper)`, `withTagIn(std::span<const String>)`, `withTagNotIn(std::span<const String>)`, `withTagIsNull()`, `withTagIsNotNull()`.
- Projection/order/group: `selectTag()`, `orderByTagAsc()`, `orderByTagDesc()`, `groupByTag()`.
- Group/count: `groupByTag()`, `groupByTagAs(alias)`, `groupByTagWithFunction(alias, function)`, `countTag()` and `countTagAs(alias)`.
- Scalar aggregate: `sumTag()` uses alias `"sum_tag"`; `sumTagAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgTag()` uses alias `"avg_tag"`; `avgTagAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minTag()` uses alias `"min_tag"`; `minTagAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxTag()` uses alias `"max_tag"`; `maxTagAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withTagContaining(text)`, `withTagStartingWith(text)`, `withTagEndingWith(text)`.

Null alias: `withTagIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTagIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withTagNotContaining(value)`.

Text predicate: `withTagNotStartingWith(value)`.

Text predicate: `withTagNotEndingWith(value)`.

Text predicate: `withTagSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
