# Field Assist: reading.payload

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `payload()` / `updatePayload(const std::optional<Json>&)`; changed `changedPayload()`; evaluation `evalPayload()`; expression `getPayload()`.
- Candidate filters: `withPayloadIsJsonMe()` and `withPayloadIsNotJsonMe()`; exact value `jsonMe()`.
- Scalar predicates: `withPayloadIs`, `withPayloadIsNot`, `withPayloadGreaterThan`, `withPayloadGreaterThanOrEqualTo`, `withPayloadLessThan`, `withPayloadLessThanOrEqualTo`. Arguments: `const Json&`.
- Range/set/null predicates: `withPayloadBetween(lower, upper)`, `withPayloadIn(std::span<const Json>)`, `withPayloadNotIn(std::span<const Json>)`, `withPayloadIsNull()`, `withPayloadIsNotNull()`.
- Projection/order/group: `selectPayload()`, `orderByPayloadAsc()`, `orderByPayloadDesc()`, `groupByPayload()`.
- Group/count: `groupByPayload()`, `groupByPayloadAs(alias)`, `groupByPayloadWithFunction(alias, function)`, `countPayload()` and `countPayloadAs(alias)`.
- Scalar aggregate: `sumPayload()` uses alias `"sum_payload"`; `sumPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPayload()` uses alias `"avg_payload"`; `avgPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPayload()` uses alias `"min_payload"`; `minPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPayload()` uses alias `"max_payload"`; `maxPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withPayloadIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPayloadIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
