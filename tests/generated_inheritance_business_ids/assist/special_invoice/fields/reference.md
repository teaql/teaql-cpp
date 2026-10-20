# Field Assist: special_invoice.reference

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `reference()` / `updateReference(std::string_view)`; changed `changedReference()`; evaluation `evalReference()`; expression `getReference()`.
- Scalar predicates: `withReferenceIs`, `withReferenceIsNot`, `withReferenceGreaterThan`, `withReferenceGreaterThanOrEqualTo`, `withReferenceLessThan`, `withReferenceLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withReferenceBetween(lower, upper)`, `withReferenceIn(std::span<const String>)`, `withReferenceNotIn(std::span<const String>)`, `withReferenceIsNull()`, `withReferenceIsNotNull()`.
- Projection/order/group: `selectReference()`, `orderByReferenceAsc()`, `orderByReferenceDesc()`, `groupByReference()`.
- Group/count: `groupByReference()`, `groupByReferenceAs(alias)`, `groupByReferenceWithFunction(alias, function)`, `countReference()` and `countReferenceAs(alias)`.
- Scalar aggregate: `sumReference()` uses alias `"sum_reference"`; `sumReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgReference()` uses alias `"avg_reference"`; `avgReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minReference()` uses alias `"min_reference"`; `minReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxReference()` uses alias `"max_reference"`; `maxReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withReferenceContaining(text)`, `withReferenceStartingWith(text)`, `withReferenceEndingWith(text)`.

Null alias: `withReferenceIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withReferenceIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withReferenceNotContaining(value)`.

Text predicate: `withReferenceNotStartingWith(value)`.

Text predicate: `withReferenceNotEndingWith(value)`.

Text predicate: `withReferenceSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
