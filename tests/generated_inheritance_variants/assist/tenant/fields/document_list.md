# Field Assist: tenant.document_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalDocumentList()`; query view handle: `documentListHandle()`.
- Reverse relation: `documentList()` returns SmartList<Document>&; query `selectDocumentList()`, `selectDocumentListWith(DocumentRequest<P>)`, `withDocumentList(DocumentRequest<P>)`; expression `getDocumentList()`.

Presence: `haveDocuments()`, `haveNoDocuments()`. Matching: `withDocumentListMatching(child)`, `withoutDocumentListMatching(child)`.

Count: `countDocuments()`, `countDocumentsAs(alias)`, `countDocumentsWith(alias, child)`. Statistics/details: `statsFromDocuments(child)`, `statsFromDocumentsAs(alias, child)`, `groupByDocumentsWithDetails(child)`.

Field statistic: `minCreatedAtOfDocuments()`, `minCreatedAtOfDocumentsAs(alias, child)`.

Field statistic: `maxCreatedAtOfDocuments()`, `maxCreatedAtOfDocumentsAs(alias, child)`.

Field statistic: `minUpdatedAtOfDocuments()`, `minUpdatedAtOfDocumentsAs(alias, child)`.

Field statistic: `maxUpdatedAtOfDocuments()`, `maxUpdatedAtOfDocumentsAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
