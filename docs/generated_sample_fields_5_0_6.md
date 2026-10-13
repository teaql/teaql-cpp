# Generated sample scalar, temporal and reference defaults

Status: partial G12/P6 qualification. Runtime:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; producer:
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Models and unchanged source

Three explicit models retain complete original output and provenance hashes:

- `sample_fields.xml` includes a quoted JSON literal. The frozen generated crate
  cannot compile because sample_data.rs emits that literal without escaping its
  quotes. `verify_sample_fields_source_failure.py --check` preserves the actual
  compiler rejection; no generated source is edited to make it compile.
- `sample_fields_runtime.xml` uses ordinary optional `jsonMe()`. It compiles, but
  the original sample setter receives Text("jsonMe() 1"), and typed checking fails.
  This runtime failure is compared directly, including the exact error and empty
  persisted rows.
- `sample_fields_temporal.xml` retains the other fields and supplies successful
  scalar/temporal execution. This separate configuration is not evidence of a
  successful full JSON-literal original crate.

The successful model covers mandatory/optional date, timestamp and formatted
clock text; true/false/optional booleans; Decimal, double and optional float;
optional integer; ordinary text; numeric defaults zero, signed plus, negative
parse failure and maximum u64 bound. A second model has two required references
to generated records, exercising reference publication, fanout and per-row
selection of unused IDs. Negative literal `-9` is inferred as String; the distinct
`integer(sampleValue=-9)` field tests numeric parse fallback.

## Corrections to the generated adapters

The template's numeric property observes inherited `isNumber` metadata. The
implementation workspace's old direct `isNumber()` method excludes float.
C++ sample generation now reads the attribute as well, so optional float receives
the same I64 source operand and typed value as the frozen template.

JSON sample input stays Text in the mutation ledger. The previous C++ recipe
wrapped it in Json, silently making the source's rejected input acceptable.
The corrected thin adapter leaves native JSON storage unchanged and marks the
field loaded through the existing indexed state, matching the source setter's
failed conversion/fallback. Public typed JSON update APIs remain available for
explicit valid values. Model-aware Assist explains the sample failure, and the
common guide explains first-error stop and retention of earlier commits.

## Controlled clock and paired evidence

A test-only Linux preload library controls CLOCK_REALTIME in each probe process.
Other clock IDs delegate to the system syscall. No TEAQL runtime target links
this fixture, and production code continues to use the actual process wall clock.
Every observation independently checks the observed wall time; comparing two
uncontrolled runs or normalizing timestamps is not allowed.

Four clocks cover the 2025 year boundary, 2024 leap day, the 2000 boundary and
123 milliseconds after the Unix epoch. Seeded past dates at that last clock
produce negative timestamps and pre-epoch calendar dates. A negative current
system clock is outside this corpus: chrono::Utc::now rejects that process-clock
stimulus, rather than supplying a comparable ordinary sample observation.

All **85** unchanged original/C++ observations match across **89** invocations
and **1,980** newly persisted rows. Coverage includes seeds zero/seven,
1094/1095 modulo boundaries and maximum-u64-minus-19, Tiny/Small fanout,
present/missing roots, repeated generation, full nullable/scalar storage and
ordered dual-reference rows. Errors, integer/real kinds, decimal storage,
calendar strings, milliseconds, IDs, dependency skips and reports are compared
without normalization. Both probes use the same explicit BOOLEAN/NUMERIC/DATE
schema declarations; this is not complete automatic bootstrap qualification.

The native module integration runs actual generated sample creation and typed
readback with a null default PMR resource. It explicitly copies a generated model
into Global, actually deletes UserContext, and verifies retained dates and nested
optional clock-text allocation. No Context lifetime is extended implicitly.

## Remaining qualification

Complete JSON-literal source generation remains rejected and explicitly recorded.
Further constant/candidate/inherited/custom-type configurations, full source trace
and sample progress log behavior, every Assist/extension responsibility and the
P7 integration matrix remain open. These observations do not close G12 or P6.

## Completed gates

- Live original/C++ replay: 85/85 observations, 89 invocations, 1,980 persisted rows.
- Native CTest: 2/2 (new field replay and existing generated sample execution).
- ASan/UBSan: 1/1 field replay, with leak scanning disabled.
- Module generated sample/readback/Global retention: 1/1.
- Java CppGenerationTest: 27/27, including numeric attribute and JSON Text adapters.
- C++ reproduction: 18 sets / 216 files; unchanged original reproduction: three
  complete new fixtures / 78 files.
- Original JSON-literal compiler rejection reproduces with `--locked` and its
  recorded diagnostic. Pinned format/check covers 979 C++ files; source locks,
  generator ledger, module catalog/path and whitespace checks pass.

The process clock library is test-only and has no sanitizer runtime dependency;
the actual C++ sample runtime/probe remains instrumented in the sanitize profile.
MIPS/QEMU and physical-device execution were not run for this change.
