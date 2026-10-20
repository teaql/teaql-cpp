# Field Assist: platform.namespace_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalNamespaceList()`; query view handle: `namespaceListHandle()`.
- Reverse relation: `namespaceList()` returns SmartList<Namespace>&; query `selectNamespaceList()`, `selectNamespaceListWith(NamespaceRequest<P>)`, `withNamespaceList(NamespaceRequest<P>)`; expression `getNamespaceList()`.

Presence: `haveNamespaces()`, `haveNoNamespaces()`. Matching: `withNamespaceListMatching(child)`, `withoutNamespaceListMatching(child)`.

Count: `countNamespaces()`, `countNamespacesAs(alias)`, `countNamespacesWith(alias, child)`. Statistics/details: `statsFromNamespaces(child)`, `statsFromNamespacesAs(alias, child)`, `groupByNamespacesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
