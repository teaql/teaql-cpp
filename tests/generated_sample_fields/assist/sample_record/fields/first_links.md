# Field Assist: sample_record.first_links

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalFirstLinks()`; query view handle: `firstLinksHandle()`.
- Reverse relation: `firstLinks()` returns SmartList<SampleLink>&; query `selectFirstLinks()`, `selectFirstLinksWith(SampleLinkRequest<P>)`, `withFirstLinks(SampleLinkRequest<P>)`; expression `getFirstLinks()`.

Presence: `haveFirstLinks()`, `haveNoFirstLinks()`. Matching: `withFirstLinksMatching(child)`, `withoutFirstLinksMatching(child)`.

Count: `countFirstLinks()`, `countFirstLinksAs(alias)`, `countFirstLinksWith(alias, child)`. Statistics/details: `statsFromFirstLinks(child)`, `statsFromFirstLinksAs(alias, child)`, `groupByFirstLinksWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
