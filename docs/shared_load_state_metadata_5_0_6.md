# Direct field-layout metadata comparison

LS01's generator numbering/revision responsibility now has 78 direct original/
C++-producer comparisons: 65 successful descriptor results and 13 matching
validation failures. G15 and full generator acceptance remain partial.

The source is generator `45e1572c4956ca963410d8ea436ead3c8857087a`:
`ObjectDescriptor.getPredefinedFieldIndexes/getPredefinedFieldLayoutRevision`
and the returned `PredefinedFieldIndex` accessors. C++ uses the actual
`CppFieldLayoutMetadata.fields/revision` methods from the independent
`cpp_support` workspace. Rust runtime remains the locked 5.0.6 commit.

## Actual implementation calls

The same Java observer constructs explicit descriptors and runs in separate
reference/implementation classpaths. Reference mode invokes the original public
APIs. C++ mode invokes the existing package-private metadata methods through
reflection; it does not copy their collection or revision algorithms.

Original indexes, canonical/member/storage/column names, field owner/type/reference
metadata and the full SHA-256 revision are compared. C++ field positions follow
the list positions that `CppCoreGenerator.emitFieldLayout` actually emits.
Exceptions retain their complete class and message after unwrapping Java's
reflection envelope. No names, SQL text, revisions or errors are normalized.

Both model class trees are compiled before observations; the C++ generator is
also compiled. The record retains command results and source/class hashes.
Only ignored build output changes in the frozen reference. Production source,
the original generator checkout and generated C++ SDKs remain unchanged.

## Source branch matrix

| Branch | Actual observations |
| --- | --- |
| Numbering and order | Fixed lists through 514 total fields, including 31/32/63/64/65/127/128/129 boundaries; ordered and reordered declarations. |
| Revision input | Empty/null/ordinary/Unicode/long domain names, absent domain owner, raw type/reference metadata and distinct SQL table/column attributes. |
| Naming/mappings | Camel/underscore/acronym/keyword/Unicode/long/NUL names, physical aliases, quoted/newline/NUL/Unicode SQL columns and actual object-FK storage names. |
| Inheritance | One through five ancestors; parent-first slots, child additions and compatible id/version/name redeclarations retain the inherited owner and metadata. Child SQL overrides do not replace a compatible inherited slot. |
| Reserved namespaces | `#` and `_` fields receive no slot, including repeated excluded fields. |
| Validation | Missing parent, one/two/three-node cycles, null/empty/Unicode-blank names, repeated canonical names, incompatible declared type/reference and repeated child overrides preserve all 13 source failures. |

These are descriptor API inputs, not claims that every input is a valid KSML
application. Explicit physical columns isolate this contract from DataService
bootstrap. The descriptor collector can accept a list without id/version;
runtime `FieldLayout::from_generated` separately requires them and has its own
44-case source comparison. Malformed KSML parsing and complete generated-artifact
acceptance remain independent responsibilities.

The [initial record](parity/shared_load_state_metadata_initial_5_0_6.json.gz)
retains an observer-construction defect: four ObjectType fixtures lacked a
referenced descriptor, so both implementations failed during attribute lookup.
Those failures were not successful object-mapping evidence. The observer now
supplies the actual referenced descriptor and checks the exact intended set of
successful/failing fixture labels. The
[final record](parity/shared_load_state_metadata_5_0_6.json.gz) compares all 78
outcomes with no differences or unexpected successes/failures. No production
algorithm was changed to make this pass.

## Reproduction

```sh
python3 scripts/verify_shared_load_state_metadata_5_0_6.py \
  --record /tmp/shared_load_state_metadata.json.gz
```

Use a fresh record path. The verifier checks the active locks, compiles the
original model and current generator offline, builds the observer, then invokes
both implementations. It does not regenerate or patch SDKs, alter Rust sources
or introduce a runtime dependency. This evidence joins LS01's full source review;
the remaining LS02-LS14 and combined acceptance gates retain their requirements.
