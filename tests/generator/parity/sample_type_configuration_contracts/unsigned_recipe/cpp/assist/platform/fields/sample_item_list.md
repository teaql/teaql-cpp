# Field Assist: platform.sample_item_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSampleItemList()`; query view handle: `sampleItemListHandle()`.
- Reverse relation: `sampleItemList()` returns SmartList<SampleItem>&; query `selectSampleItemList()`, `selectSampleItemListWith(SampleItemRequest<P>)`, `withSampleItemList(SampleItemRequest<P>)`; expression `getSampleItemList()`.

Presence: `haveSampleItems()`, `haveNoSampleItems()`. Matching: `withSampleItemListMatching(child)`, `withoutSampleItemListMatching(child)`.

Count: `countSampleItems()`, `countSampleItemsAs(alias)`, `countSampleItemsWith(alias, child)`. Statistics/details: `statsFromSampleItems(child)`, `statsFromSampleItemsAs(alias, child)`, `groupBySampleItemsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
