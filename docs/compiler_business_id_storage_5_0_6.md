# Compiler-resolved Business ID storage

## Confirmed gap

The [actual v3 controller diagnosis](parity/compiler_business_id_storage_diagnosis_5_0_6.json.gz)
uses the unchanged `tests/generator/business_ids.xml` model. Public evaluation,
complete original generation and original compilation succeed. MIR, complete
descriptor observation and scalar observation also succeed. C++ generation then
exits 1 with `UNMAPPED_TYPE` for `receipt.receipt_number`; no C++ SDK is published.

The original getter returns the generated strong `ReceiptNumber` type; Ticket's
getter similarly returns `TicketNumber`. The v3 scalar observer recognizes
ordinary TeaQL scalar identities, so it records both wrappers as `unmapped:`.
The C++ producer compares that fact with its String storage carrier and refuses.
Existing C++ Business ID types and runtime services already supply the intended
abstraction. This is an incomplete compiler mapping, not a Rust reference defect
or an additional compatibility exception.

The original wrapper has a public string view and `From<&Wrapper> for Value`;
its underlying entity property is String. The canonical constructor is private
to the original crate and must not become an observer dependency. Wrapper names
must come from the locked `RustFieldDescriptorNaming.rustBusinessIdTypeName`
helper and owning module, preserving distinct strong identities. The observer
must use the public module re-export, since the `entity` submodule is private.

## Implementation plan

1. Add an explicit, versioned semantic-storage observation. Preserve the existing
   capability/document formats and distinguish actual wrapper identity, semantic
   Business ID responsibility and its String representation. Do not relabel the
   wrapper as a primitive or serialize process-specific TypeId numbers.
2. Derive the wrapper identity from authentic locked metadata/naming helpers.
   Bind the getter's TypeId to that generated wrapper in the same original
   library, with compiler-checked public string/value conversion contracts.
   An arbitrary unknown type with a similar method must remain unmapped.
3. Carry complete source-owned facts through the worker and generation-scoped
   resolver. Validate model/library/field membership and strict protocol members.
   Keep C++ String storage and existing strong Business ID application APIs;
   preserve the independent allocator, profile, business clock and Context SPIs.
4. Verify complete original/C++ generation and actual typed Business ID reads,
   Q/E/JSON and relevant allocation/persistence consumers. Include prefix,
   namespace, profile, inheritance and invalid witness/type/membership controls.
   Retain explicit original compiler refusals and unrelated unsupported types.
5. Run current Java/worker contracts, retained SDK reproduction and affected
   native checks. Qualify ordinary/optimized validators and exact source epochs,
   remove owned disposable outputs, record English evidence and publish.

The successful diagnostic removes its parent and worker outputs after recording
raw commands, compiler output, scalar facts and the exact C++ failure. It adds
no C++ runtime dependency or ownership domain. The pending Option-model and
column-attribute choices, remaining configuration mappings, G13/G14/G17 and
all outstanding P0-P7 exits retain their full scope.
