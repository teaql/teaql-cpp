# Field Assist: platform.record_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalRecordList()`; query view handle: `recordListHandle()`.
- Reverse relation: `recordList()` returns SmartList<Record>&; query `selectRecordList()`, `selectRecordListWith(RecordRequest<P>)`, `withRecordList(RecordRequest<P>)`; expression `getRecordList()`.

Presence: `haveRecords()`, `haveNoRecords()`. Matching: `withRecordListMatching(child)`, `withoutRecordListMatching(child)`.

Count: `countRecords()`, `countRecordsAs(alias)`, `countRecordsWith(alias, child)`. Statistics/details: `statsFromRecords(child)`, `statsFromRecordsAs(alias, child)`, `groupByRecordsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
