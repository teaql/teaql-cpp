# Field Assist: tenant.ticket_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalTicketList()`; query view handle: `ticketListHandle()`.
- Reverse relation: `ticketList()` returns SmartList<Ticket>&; query `selectTicketList()`, `selectTicketListWith(TicketRequest<P>)`, `withTicketList(TicketRequest<P>)`; expression `getTicketList()`.

Presence: `haveTickets()`, `haveNoTickets()`. Matching: `withTicketListMatching(child)`, `withoutTicketListMatching(child)`.

Count: `countTickets()`, `countTicketsAs(alias)`, `countTicketsWith(alias, child)`. Statistics/details: `statsFromTickets(child)`, `statsFromTicketsAs(alias, child)`, `groupByTicketsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
