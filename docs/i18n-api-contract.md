# Language and catalog source contract

Baseline: Rust v5.0.5 commit `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete language.rs (215 lines) and i18n.rs (342 lines) bodies are reviewed,
including public capabilities, private parsing/lookup/rendering and source tests.
This is a responsibility review, not complete runtime acceptance.

| Source responsibility | Native mapping and evidence |
| --- | --- |
| Fifteen Language variants and Locale alias | Same declared order, English default, ordinary enum copy/equality/order/hash and all_languages constant. Existing code/alias corpus and new derived Debug observations. |
| code / from_code | Canonical code, Unicode trimming, underscore replacement, ASCII lowercase and exact Chinese/Filipino exceptions plus supported base-code fallback. Existing actual-call corpus and unchanged locale fixture. |
| MessageTranslator / BuiltinTranslator | Separate translator interface, explicit selected output allocator, language accessor and scalar copy. Source Debug language field is restored; a native polymorphic copy preserves the value rather than requiring a bytewise copy of a vtable. |
| Free check/location translation | Builtin catalog dispatch and Unicode identifier/path title casing, including indexed path segments. Both source title helpers are reviewed; existing actual calls remain unchanged. |
| Immutable I18nCatalog fields | Explicitly owned default locale and ordered locale maps. Immutable generated builtin facts replace a process-static Arc owning an allocated catalog. |
| LocaleCatalog Default/Clone/Debug | Empty messages/vocabulary allocation, destination copies and complete ordered private map diagnostics. It is a private implementation type, not a new application SPI. |
| from_json / private parse | Exact three root fields/schema, supported default/canonical locale keys, exact two locale fields, nonempty strings, required default locale and source template validation. Existing public-method corpus/source fixture ports. |
| builtin / fallback | Source builtin has no fallback; parsed custom catalogs have builtin fallback. The native flags retain this responsibility without extending a User lifetime through shared ownership. |
| message / vocabulary / lookup_exact / lookup | Requested custom locale, requested builtin locale, custom default locale, builtin default locale, then raw key, preserving source order. Vocabulary allows literal braces. |
| CheckResult translation | Eight rule keys; location/system/input/input_len arguments; exact Value formatting and Unicode scalar length; replacement order remains source order. Existing 4,575 actual-call inputs. |
| Derived catalog Debug/Clone | default_locale, complete locales and optional full builtin fallback in source order and structure. New original-call corpus includes cloned diagnostics in nine modes. |
| Private validation/render helpers | All four allowed placeholders and nested/unclosed/unmatched/unknown errors; sequential replacement; title casing and Value formatting are reviewed against their native counterparts. |

There is no source Language/catalog serde derive. Fixture JSON envelopes observe
diagnostics and method results; they do not introduce a new production wire
protocol. Hash bucket algorithms are native rather than a persisted Rust hash
format. Returned strings own their selected destination; they are not catalog
views. Scoped borrowing, owner-thread Context access and explicit Global copies
are documented native lifetime adaptations. Thread transfer through Rust Arc
does not silently extend native User data.

## Reproduced derived diagnostic omission

A public runtime-facade compile fixture fails to find Language, BuiltinTranslator
and I18nCatalog Debug. runtime/i18n_debug.hpp and i18n_debug.cpp restore each
original derive and retain the private LocaleCatalog shape. Enum diagnostics
show variant names; translator diagnostics show its language field. Catalog
diagnostics show source fields, not native builtin/fallback storage flags.

Builtin formatting groups immutable facts into ordered temporary maps/vectors
using the DebugWriter output allocator. No default resource, hidden catalog
owner or Context cache is added. Generated immutable tables are unchanged.
Custom diagnostics borrow their owner-local maps only for the formatting call.
Source Arc Debug prints its payload; native fallback formatting likewise prints
the complete builtin catalog inside Some, with no invented ownership wrapper.

## Original-call and lifecycle evidence

The independently named i18n_debug_scope oracle invokes actual source Debug,
Default and Clone. Sixty inputs combine fifteen languages with builtin, empty
custom, mixed-language/custom-default and Unicode vocabulary catalogs. Every
observation retains six values in nine formatter modes: language, default,
translator, translator clone, catalog and catalog clone. All 60 observations
match live original Rust on standalone native C++ with no normalization.

The existing native ownership contract passes after adding source diagnostics.
With default PMR disabled, it deletes the source User, formats a retained Global
catalog into the next User, checks its complete long vocabulary/private fallback
and destination allocator, and retains the earlier translation/error/module
ownership checks. The public facade compile omission is repaired independently.

Fresh named sanitizer/MIPS builds include original translation/ownership tests,
the existing 4,575-input translation probe and the new diagnostic probe. Their
terminal results remain required, followed by the current isolated full native
consumer refresh/regression. The source group and stages 2–6 remain open;
these diagnostic repairs do not accept graph-save translation call sites,
arbitrary custom translators or complete runtime composition.

The fresh MIPS build finishes all forty tasks, its final refresh reports no
work, and all 309 compiled dependency records are nonempty. The selected
ownership/translation/diagnostic checks pass 3/3 in 21.47 seconds, including
the unchanged 4,575 translation inputs and sixty new diagnostics. Sanitizer
and complete isolated native consumer integration remain pending.

The fresh sanitizer build also finishes all forty tasks, its final refresh
reports no work, and all 100 compiled dependency records are nonempty. The same
three checks pass 3/3 in 37.31 seconds with ASan/UBSan and leak scanning disabled.
Selected platform acceptance is complete for this repair. The isolated full
native build still requires its final latest-header/new-source refresh and
complete regression; source-group and stage acceptance remain open.

The freshly linked isolated native consumers now pass the same three
ownership/translation/diagnostic checks 3/3 in 10.45 seconds. This is a scoped
consumer result from the current isolated directory; the whole build is still
running, so final complete native regression remains required.

## Current Context-layout review, 2026-10-08

The complete production/source review is checked again against the current
Context layout. Canonical locale normalization, both title helpers, every
parser/validation/lookup/rendering branch, source enum/default/copy/order/hash
responsibilities, translator dispatch and private catalog derives remain mapped.
The stateless native builtin tables retain the source fallback semantics without
owning Context data globally. Context installation and module/language forwarding
retain their separate responsibilities.

Fresh current targets include catalog ownership, the 4,575 translation scripts,
sixty diagnostic inputs and the combined original checker/locale source-test
port. Those builds are running on sanitizer/MIPS. The active full native build
will include the same targets; complete 322-check regression remains required.
No new source/header edit is made while that stable build runs. Source-group
and Stage 4 acceptance remain open until the current evidence is terminal.

The current four-target builds finish: sanitizer 4/4 in 11.10 seconds with leak
scanning disabled and MIPS/QEMU 4/4 in 7.20 seconds. These include the combined
original checker/locale source-test port. All 4,575 translation and sixty
diagnostic MIPS observations match live original Rust without normalization.
The stable whole-native build remains running; complete 322-check integration
and formal source acceptance remain pending. Evidence is retained in
measurements/parity/2026-10-08-i18n-current-review.

## Complete source-group acceptance, 2026-10-08

The stable complete native build finishes all 602 tasks, reports no work and
passes 322/322 in 119.11 seconds with 390 nonempty dependency records. Current
named sanitizer/MIPS selections and live original observations above complete
the public/default/trait/protocol/ownership review. This source group is accepted
within its documented Context ownership and supported-platform adaptations.
Earlier pending statements describe prior checkpoints. Complete Context,
remaining entity/runtime/save/privacy and stages 4–6 retain independent gates.
