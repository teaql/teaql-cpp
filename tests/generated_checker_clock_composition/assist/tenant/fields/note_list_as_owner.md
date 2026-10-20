# Field Assist: tenant.note_list_as_owner

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalNoteListAsOwner()`; query view handle: `noteListAsOwnerHandle()`.
- Reverse relation: `noteListAsOwner()` returns SmartList<Note>&; query `selectNoteListAsOwner()`, `selectNoteListAsOwnerWith(NoteRequest<P>)`, `withNoteListAsOwner(NoteRequest<P>)`; expression `getNoteListAsOwner()`.

Presence: `haveNotesAsOwner()`, `haveNoNotesAsOwner()`. Matching: `withNoteListAsOwnerMatching(child)`, `withoutNoteListAsOwnerMatching(child)`.

Count: `countNotesAsOwner()`, `countNotesAsOwnerAs(alias)`, `countNotesAsOwnerWith(alias, child)`. Statistics/details: `statsFromNotesAsOwner(child)`, `statsFromNotesAsOwnerAs(alias, child)`, `groupByNotesAsOwnerWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
