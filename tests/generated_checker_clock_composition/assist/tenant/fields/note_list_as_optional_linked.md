# Field Assist: tenant.note_list_as_optional_linked

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalNoteListAsOptionalLinked()`; query view handle: `noteListAsOptionalLinkedHandle()`.
- Reverse relation: `noteListAsOptionalLinked()` returns SmartList<Note>&; query `selectNoteListAsOptionalLinked()`, `selectNoteListAsOptionalLinkedWith(NoteRequest<P>)`, `withNoteListAsOptionalLinked(NoteRequest<P>)`; expression `getNoteListAsOptionalLinked()`.

Presence: `haveNotesAsOptionalLinked()`, `haveNoNotesAsOptionalLinked()`. Matching: `withNoteListAsOptionalLinkedMatching(child)`, `withoutNoteListAsOptionalLinkedMatching(child)`.

Count: `countNotesAsOptionalLinked()`, `countNotesAsOptionalLinkedAs(alias)`, `countNotesAsOptionalLinkedWith(alias, child)`. Statistics/details: `statsFromNotesAsOptionalLinked(child)`, `statsFromNotesAsOptionalLinkedAs(alias, child)`, `groupByNotesAsOptionalLinkedWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
