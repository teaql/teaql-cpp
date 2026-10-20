# Field Assist: note.optional_payload

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalPayload()` / `updateOptionalPayload(const std::optional<Json>&)`; changed `changedOptionalPayload()`; evaluation `evalOptionalPayload()`; expression `getOptionalPayload()`.
- Candidate filters: `withOptionalPayloadIsJsonMeCreateFunctionNow()` and `withOptionalPayloadIsNotJsonMeCreateFunctionNow()`; exact value `jsonMe(createFunction=now`.
- Candidate filters: `withOptionalPayloadIsUpdateFunctionNow()` and `withOptionalPayloadIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalPayloadIs`, `withOptionalPayloadIsNot`, `withOptionalPayloadGreaterThan`, `withOptionalPayloadGreaterThanOrEqualTo`, `withOptionalPayloadLessThan`, `withOptionalPayloadLessThanOrEqualTo`. Arguments: `const Json&`.
- Range/set/null predicates: `withOptionalPayloadBetween(lower, upper)`, `withOptionalPayloadIn(std::span<const Json>)`, `withOptionalPayloadNotIn(std::span<const Json>)`, `withOptionalPayloadIsNull()`, `withOptionalPayloadIsNotNull()`.
- Projection/order/group: `selectOptionalPayload()`, `orderByOptionalPayloadAsc()`, `orderByOptionalPayloadDesc()`, `groupByOptionalPayload()`.
- Group/count: `groupByOptionalPayload()`, `groupByOptionalPayloadAs(alias)`, `groupByOptionalPayloadWithFunction(alias, function)`, `countOptionalPayload()` and `countOptionalPayloadAs(alias)`.
- Scalar aggregate: `sumOptionalPayload()` uses alias `"sum_optional_payload"`; `sumOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalPayload()` uses alias `"avg_optional_payload"`; `avgOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalPayload()` uses alias `"min_optional_payload"`; `minOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalPayload()` uses alias `"max_optional_payload"`; `maxOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withOptionalPayloadIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalPayloadIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
