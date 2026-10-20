# Field Assist: platform.profile_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalProfileList()`; query view handle: `profileListHandle()`.
- Reverse relation: `profileList()` returns SmartList<Profile>&; query `selectProfileList()`, `selectProfileListWith(ProfileRequest<P>)`, `withProfileList(ProfileRequest<P>)`; expression `getProfileList()`.

Presence: `haveProfiles()`, `haveNoProfiles()`. Matching: `withProfileListMatching(child)`, `withoutProfileListMatching(child)`.

Count: `countProfiles()`, `countProfilesAs(alias)`, `countProfilesWith(alias, child)`. Statistics/details: `statsFromProfiles(child)`, `statsFromProfilesAs(alias, child)`, `groupByProfilesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
