# Field Assist: person.profile

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalProfile()`; query view handle: `profileHandle()`.
- Reverse relation: `profile()` returns const Profile*; query `selectProfile()`, `selectProfileWith(ProfileRequest<P>)`, `withProfile(ProfileRequest<P>)`; expression `getProfile()`.

Presence: `haveProfile()`, `haveNoProfile()`. Matching: `withProfileMatching(child)`, `withoutProfileMatching(child)`.

[Model/action index](../../../ASSIST.md)
