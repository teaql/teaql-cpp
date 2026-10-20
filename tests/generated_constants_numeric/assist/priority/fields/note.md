# Field Assist: priority.note

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Candidate filters: `withNoteIsFirstNote()` and `withNoteIsNotFirstNote()`; exact value `First note`.
- Candidate filters: `withNoteIsSecondNote()` and `withNoteIsNotSecondNote()`; exact value `Second note`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `"sum_note"`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `"avg_note"`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `"min_note"`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `"max_note"`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.

Null alias: `withNoteIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withNoteIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withNoteNotContaining(value)`.

Text predicate: `withNoteNotStartingWith(value)`.

Text predicate: `withNoteNotEndingWith(value)`.

Text predicate: `withNoteSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
