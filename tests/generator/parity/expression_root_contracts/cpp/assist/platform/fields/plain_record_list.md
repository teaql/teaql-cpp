# Field Assist: platform.plain_record_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalPlainRecordList()`; query view handle: `plainRecordListHandle()`.
- Reverse relation: `plainRecordList()` returns SmartList<PlainRecord>&; query `selectPlainRecordList()`, `selectPlainRecordListWith(PlainRecordRequest<P>)`, `withPlainRecordList(PlainRecordRequest<P>)`; expression `getPlainRecordList()`.

Presence: `havePlainRecords()`, `haveNoPlainRecords()`. Matching: `withPlainRecordListMatching(child)`, `withoutPlainRecordListMatching(child)`.

Count: `countPlainRecords()`, `countPlainRecordsAs(alias)`, `countPlainRecordsWith(alias, child)`. Statistics/details: `statsFromPlainRecords(child)`, `statsFromPlainRecordsAs(alias, child)`, `groupByPlainRecordsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
