# Field Assist: entry.notes

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `notes()` / `updateNotes(const std::optional<String>&)`; changed `changedNotes()`; evaluation `evalNotes()`; expression `getNotes()`.
- Scalar predicates: `withNotesIs`, `withNotesIsNot`, `withNotesGreaterThan`, `withNotesGreaterThanOrEqualTo`, `withNotesLessThan`, `withNotesLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNotesBetween(lower, upper)`, `withNotesIn(std::span<const String>)`, `withNotesNotIn(std::span<const String>)`, `withNotesIsNull()`, `withNotesIsNotNull()`.
- Projection/order/group: `selectNotes()`, `orderByNotesAsc()`, `orderByNotesDesc()`, `groupByNotes()`.
- Group/count: `groupByNotes()`, `groupByNotesAs(alias)`, `groupByNotesWithFunction(alias, function)`, `countNotes()` and `countNotesAs(alias)`.
- Scalar aggregate: `sumNotes()` uses alias `"sum_notes"`; `sumNotesAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNotes()` uses alias `"avg_notes"`; `avgNotesAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNotes()` uses alias `"min_notes"`; `minNotesAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNotes()` uses alias `"max_notes"`; `maxNotesAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNotesContaining(text)`, `withNotesStartingWith(text)`, `withNotesEndingWith(text)`.

Null alias: `withNotesIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withNotesIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
