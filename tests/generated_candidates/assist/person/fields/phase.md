# Field Assist: person.phase

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `phase()` / `updatePhase(std::string_view)`; changed `changedPhase()`; evaluation `evalPhase()`; expression `getPhase()`.
- Candidate filters: `whosePhaseIsDraft()` and `whosePhaseIsNotDraft()`; exact value `Draft`.
- Candidate filters: `whosePhaseIsInProgress()` and `whosePhaseIsNotInProgress()`; exact value `InProgress`.
- Candidate filters: `whosePhaseIsDone()` and `whosePhaseIsNotDone()`; exact value `done`.
- Scalar predicates: `whosePhaseIs`, `whosePhaseIsNot`, `whosePhaseGreaterThan`, `whosePhaseGreaterThanOrEqualTo`, `whosePhaseLessThan`, `whosePhaseLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `whosePhaseBetween(lower, upper)`, `whosePhaseIn(std::span<const String>)`, `whosePhaseNotIn(std::span<const String>)`, `whosePhaseIsNull()`, `whosePhaseIsNotNull()`.
- Projection/order/group: `selectPhase()`, `orderByPhaseAsc()`, `orderByPhaseDesc()`, `groupByPhase()`.
- Group/count: `groupByPhase()`, `groupByPhaseAs(alias)`, `groupByPhaseWithFunction(alias, function)`, `countPhase()` and `countPhaseAs(alias)`.
- Scalar aggregate: `sumPhase()` uses alias `"sum_phase"`; `sumPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPhase()` uses alias `"avg_phase"`; `avgPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPhase()` uses alias `"min_phase"`; `minPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPhase()` uses alias `"max_phase"`; `maxPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `whosePhaseContains(text)`, `whosePhaseStartsWith(text)`, `whosePhaseEndsWith(text)`.

Null alias: `whosePhaseIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whosePhaseIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `whosePhaseNotContains(value)`.

Text predicate: `whosePhaseNotStartsWith(value)`.

Text predicate: `whosePhaseNotEndsWith(value)`.

Text predicate: `whosePhaseSoundsLike(value)`.

[Model/action index](../../../ASSIST.md)
