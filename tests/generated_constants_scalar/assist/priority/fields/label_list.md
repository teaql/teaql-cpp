# Field Assist: priority.label_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalLabelList()`; query view handle: `labelListHandle()`.
- Reverse relation: `labelList()` returns SmartList<Label>&; query `selectLabelList()`, `selectLabelListWith(LabelRequest<P>)`, `withLabelList(LabelRequest<P>)`; expression `getLabelList()`.

Presence: `haveLabels()`, `haveNoLabels()`. Matching: `withLabelListMatching(child)`, `withoutLabelListMatching(child)`.

Count: `countLabels()`, `countLabelsAs(alias)`, `countLabelsWith(alias, child)`. Statistics/details: `statsFromLabels(child)`, `statsFromLabelsAs(alias, child)`, `groupByLabelsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
