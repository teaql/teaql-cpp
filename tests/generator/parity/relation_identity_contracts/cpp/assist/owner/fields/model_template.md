# Field Assist: owner.template

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalModelTemplate()`; query view handle: `modelTemplateHandle()`.
- Reverse relation: `modelTemplate()` returns SmartList<Entry>&; query `selectModelTemplate()`, `selectModelTemplateWith(EntryRequest<P>)`, `withModelTemplate(EntryRequest<P>)`; expression `getModelTemplate()`.

Presence: `haveTemplate()`, `haveNoTemplate()`. Matching: `withTemplateMatching(child)`, `withoutTemplateMatching(child)`.

Count: `countTemplate()`, `countTemplateAs(alias)`, `countTemplateWith(alias, child)`. Statistics/details: `statsFromTemplate(child)`, `statsFromTemplateAs(alias, child)`, `groupByTemplateWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
