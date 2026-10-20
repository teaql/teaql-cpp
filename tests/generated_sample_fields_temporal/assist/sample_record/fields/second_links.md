# Field Assist: sample_record.second_links

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSecondLinks()`; query view handle: `secondLinksHandle()`.
- Reverse relation: `secondLinks()` returns SmartList<SampleLink>&; query `selectSecondLinks()`, `selectSecondLinksWith(SampleLinkRequest<P>)`, `withSecondLinks(SampleLinkRequest<P>)`; expression `getSecondLinks()`.

Presence: `haveSecondLinks()`, `haveNoSecondLinks()`. Matching: `withSecondLinksMatching(child)`, `withoutSecondLinksMatching(child)`.

Count: `countSecondLinks()`, `countSecondLinksAs(alias)`, `countSecondLinksWith(alias, child)`. Statistics/details: `statsFromSecondLinks(child)`, `statsFromSecondLinksAs(alias, child)`, `groupBySecondLinksWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
