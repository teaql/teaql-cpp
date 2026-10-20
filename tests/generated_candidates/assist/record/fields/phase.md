# Field Assist: record.phase

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `phase()` / `updatePhase(std::string_view)`; changed `changedPhase()`; evaluation `evalPhase()`; expression `getPhase()`.
- Candidate filters: `withPhaseIsDraft()` and `withPhaseIsNotDraft()`; exact value `Draft`.
- Candidate filters: `withPhaseIsInProgress()` and `withPhaseIsNotInProgress()`; exact value `InProgress`.
- Candidate filters: `withPhaseIsDone()` and `withPhaseIsNotDone()`; exact value `done`.
- Scalar predicates: `withPhaseIs`, `withPhaseIsNot`, `withPhaseGreaterThan`, `withPhaseGreaterThanOrEqualTo`, `withPhaseLessThan`, `withPhaseLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withPhaseBetween(lower, upper)`, `withPhaseIn(std::span<const String>)`, `withPhaseNotIn(std::span<const String>)`, `withPhaseIsNull()`, `withPhaseIsNotNull()`.
- Projection/order/group: `selectPhase()`, `orderByPhaseAsc()`, `orderByPhaseDesc()`, `groupByPhase()`.
- Group/count: `groupByPhase()`, `groupByPhaseAs(alias)`, `groupByPhaseWithFunction(alias, function)`, `countPhase()` and `countPhaseAs(alias)`.
- Scalar aggregate: `sumPhase()` uses alias `"sum_phase"`; `sumPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPhase()` uses alias `"avg_phase"`; `avgPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPhase()` uses alias `"min_phase"`; `minPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPhase()` uses alias `"max_phase"`; `maxPhaseAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withPhaseContaining(text)`, `withPhaseStartingWith(text)`, `withPhaseEndingWith(text)`.

Null alias: `withPhaseIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPhaseIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withPhaseNotContaining(value)`.

Text predicate: `withPhaseNotStartingWith(value)`.

Text predicate: `withPhaseNotEndingWith(value)`.

Text predicate: `withPhaseSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
