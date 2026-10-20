# Field Assist: platform.template_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalTemplateList()`; query view handle: `templateListHandle()`.
- Reverse relation: `templateList()` returns SmartList<Template>&; query `selectTemplateList()`, `selectTemplateListWith(TemplateRequest<P>)`, `withTemplateList(TemplateRequest<P>)`; expression `getTemplateList()`.

Presence: `haveTemplates()`, `haveNoTemplates()`. Matching: `withTemplateListMatching(child)`, `withoutTemplateListMatching(child)`.

Count: `countTemplates()`, `countTemplatesAs(alias)`, `countTemplatesWith(alias, child)`. Statistics/details: `statsFromTemplates(child)`, `statsFromTemplatesAs(alias, child)`, `groupByTemplatesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
