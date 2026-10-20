# Field Assist: owner.class

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalModelClass()`; query view handle: `modelClassHandle()`.
- Reverse relation: `modelClass()` returns SmartList<Entry>&; query `selectModelClass()`, `selectModelClassWith(EntryRequest<P>)`, `withModelClass(EntryRequest<P>)`; expression `getModelClass()`.

Presence: `haveClass()`, `haveNoClass()`. Matching: `withClassMatching(child)`, `withoutClassMatching(child)`.

Count: `countClass()`, `countClassAs(alias)`, `countClassWith(alias, child)`. Statistics/details: `statsFromClass(child)`, `statsFromClassAs(alias, child)`, `groupByClassWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
