# Field Assist: tenant.receipt_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalReceiptList()`; query view handle: `receiptListHandle()`.
- Reverse relation: `receiptList()` returns SmartList<Receipt>&; query `selectReceiptList()`, `selectReceiptListWith(ReceiptRequest<P>)`, `withReceiptList(ReceiptRequest<P>)`; expression `getReceiptList()`.

Presence: `haveReceipts()`, `haveNoReceipts()`. Matching: `withReceiptListMatching(child)`, `withoutReceiptListMatching(child)`.

Count: `countReceipts()`, `countReceiptsAs(alias)`, `countReceiptsWith(alias, child)`. Statistics/details: `statsFromReceipts(child)`, `statsFromReceiptsAs(alias, child)`, `groupByReceiptsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
