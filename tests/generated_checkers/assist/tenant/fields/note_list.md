# Field Assist: tenant.note_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalNoteList()`; query view handle: `noteListHandle()`.
- Reverse relation: `noteList()` returns SmartList<Note>&; query `selectNoteList()`, `selectNoteListWith(NoteRequest<P>)`, `withNoteList(NoteRequest<P>)`; expression `getNoteList()`.

Presence: `haveNotes()`, `haveNoNotes()`. Matching: `withNoteListMatching(child)`, `withoutNoteListMatching(child)`.

Count: `countNotes()`, `countNotesAs(alias)`, `countNotesWith(alias, child)`. Statistics/details: `statsFromNotes(child)`, `statsFromNotesAs(alias, child)`, `groupByNotesWithDetails(child)`.

Field statistic: `minCreatedAtOfNotes()`, `minCreatedAtOfNotesAs(alias, child)`.

Field statistic: `maxCreatedAtOfNotes()`, `maxCreatedAtOfNotesAs(alias, child)`.

Field statistic: `minUpdatedAtOfNotes()`, `minUpdatedAtOfNotesAs(alias, child)`.

Field statistic: `maxUpdatedAtOfNotes()`, `maxUpdatedAtOfNotesAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
