# Field Assist: ticket.ticket_number

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `ticketNumber()` / `updateTicketNumber(const TicketNumber&)`; changed `changedTicketNumber()`; evaluation `evalTicketNumber()`; expression `getTicketNumber()`.
- Scalar predicates: `withTicketNumberIs`, `withTicketNumberIsNot`, `withTicketNumberGreaterThan`, `withTicketNumberGreaterThanOrEqualTo`, `withTicketNumberLessThan`, `withTicketNumberLessThanOrEqualTo`. Arguments: `const TicketNumber&`.
- Range/set/null predicates: `withTicketNumberBetween(lower, upper)`, `withTicketNumberIn(std::span<const String>)`, `withTicketNumberNotIn(std::span<const String>)`, `withTicketNumberIsNull()`, `withTicketNumberIsNotNull()`.
- Projection/order/group: `selectTicketNumber()`, `orderByTicketNumberAsc()`, `orderByTicketNumberDesc()`, `groupByTicketNumber()`.
- Group/count: `groupByTicketNumber()`, `groupByTicketNumberAs(alias)`, `groupByTicketNumberWithFunction(alias, function)`, `countTicketNumber()` and `countTicketNumberAs(alias)`.
- Scalar aggregate: `sumTicketNumber()` uses alias `"sum_ticket_number"`; `sumTicketNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgTicketNumber()` uses alias `"avg_ticket_number"`; `avgTicketNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minTicketNumber()` uses alias `"min_ticket_number"`; `minTicketNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxTicketNumber()` uses alias `"max_ticket_number"`; `maxTicketNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withTicketNumberContaining(text)`, `withTicketNumberStartingWith(text)`, `withTicketNumberEndingWith(text)`.

Null alias: `withTicketNumberIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTicketNumberIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
