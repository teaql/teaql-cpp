# Field Assist: platform.operator_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalOperatorList()`; query view handle: `operatorListHandle()`.
- Reverse relation: `operatorList()` returns SmartList<Operator>&; query `selectOperatorList()`, `selectOperatorListWith(OperatorRequest<P>)`, `withOperatorList(OperatorRequest<P>)`; expression `getOperatorList()`.

Presence: `haveOperators()`, `haveNoOperators()`. Matching: `withOperatorListMatching(child)`, `withoutOperatorListMatching(child)`.

Count: `countOperators()`, `countOperatorsAs(alias)`, `countOperatorsWith(alias, child)`. Statistics/details: `statsFromOperators(child)`, `statsFromOperatorsAs(alias, child)`, `groupByOperatorsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
