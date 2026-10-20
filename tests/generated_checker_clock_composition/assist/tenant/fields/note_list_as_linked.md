# Field Assist: tenant.note_list_as_linked

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalNoteListAsLinked()`; query view handle: `noteListAsLinkedHandle()`.
- Reverse relation: `noteListAsLinked()` returns SmartList<Note>&; query `selectNoteListAsLinked()`, `selectNoteListAsLinkedWith(NoteRequest<P>)`, `withNoteListAsLinked(NoteRequest<P>)`; expression `getNoteListAsLinked()`.

Presence: `haveNotesAsLinked()`, `haveNoNotesAsLinked()`. Matching: `withNoteListAsLinkedMatching(child)`, `withoutNoteListAsLinkedMatching(child)`.

Count: `countNotesAsLinked()`, `countNotesAsLinkedAs(alias)`, `countNotesAsLinkedWith(alias, child)`. Statistics/details: `statsFromNotesAsLinked(child)`, `statsFromNotesAsLinkedAs(alias, child)`, `groupByNotesAsLinkedWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
