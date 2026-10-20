# Field Assist: json_graph_row.single_child

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSingleChild()`; query view handle: `singleChildHandle()`.
- Reverse relation: `singleChild()` returns const JsonGraphRow*; query `selectSingleChild()`, `selectSingleChildWith(JsonGraphRowRequest<P>)`, `withSingleChild(JsonGraphRowRequest<P>)`; expression `getSingleChild()`.

Presence: `haveSingleChild()`, `haveNoSingleChild()`. Matching: `withSingleChildMatching(child)`, `withoutSingleChildMatching(child)`.

[Model/action index](../../../ASSIST.md)
