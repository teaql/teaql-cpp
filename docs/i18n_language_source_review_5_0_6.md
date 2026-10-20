# Complete Language and I18n source review: Rust 5.0.6

Status: both complete sources are accepted under LI01-LI12 after installed
native/imported integration and durable original-source reproduction.
All 215 lines of `language.rs` and 342 lines of `i18n.rs` are reviewed below.
The four unchanged original unit tests pass, and **4,635** complete observations
match the actual original and imported implementations without normalization.
The entire existing C++ lifetime test also passes through a temporary canonical
module consumer at the earlier checkpoint. The installed continuation below
advances source dispositions to **32 accepted / 90 pending / 2 excluded_external**.

The reference remains Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Neither original source, C++ implementation, generated SDK nor committed module
configuration changes in this review. It adds no runtime dependency.

## Complete source responsibilities

| ID | Source contract | Mapping and reviewed boundary |
| --- | --- | --- |
| LI01 | All fifteen Language variants, ALL, default, copy/equality/order/hash and Locale alias | Preserve the declared enum order, English zero/default, canonical codes and the alias. Ordinary C++ enum operations carry the value traits. BuiltinTranslator retains only the source-defined explicit language constructor and immutable enum state. |
| LI02 | Locale normalization and alias selection | Rust whitespace trimming, underscore replacement and ASCII case folding precede exact Chinese/Traditional Chinese/Filipino aliases and ordinary primary-language matching. `zh-Latn` and empty input remain unsupported; `tl` does not imply support for arbitrary `tl-*` codes. |
| LI03 | Translator protocol and builtin delegation | Keep MessageTranslator's language and translation responsibilities separate. BuiltinTranslator and free translation call the builtin catalog. The language argument does not localize the existing title-case location helper. Context installation remains an independently checked resource boundary. |
| LI04 | Complete public/private catalog layouts and derive responsibilities | I18nCatalog retains default locale, sorted locale maps and optional builtin fallback semantics. LocaleCatalog retains independent messages/vocabulary maps and source empty defaults. Explicit-owner copying replaces Arc/Clone retention; raw Debug preserves source private maps and fallback fields. No Rust PartialEq or public I18nCatalog Default trait is invented. |
| LI05 | Root/schema/default locale and complete entry parsing | Require exactly schema/defaultLocale/locales, the exact schema string, a supported default locale, canonical locale keys, exact messages/vocabulary entry keys and nonempty string translations. Whitespace-only translations and arbitrary string keys retain source behavior. The default locale must exist. JSON-object uniqueness plus canonical-key validation makes the later duplicate-canonical insertion branch unreachable for valid parsed objects; it is not a new deduplication policy. |
| LI06 | Complete placeholder validation | Validate messages against location/system/input/input_len. Reject nested opening, unclosed placeholders, unmatched closing and unknown names. Vocabulary values do not acquire message-template validation. Parsing fails before a replacement catalog is installed. |
| LI07 | Builtin catalog and fallback ownership | Source OnceLock/Arc becomes stateless immutable builtin tables plus an explicitly owned catalog view. No process-owned Context payload or hidden cache is introduced. Custom catalogs acquire the same builtin fallback semantics; parser failure does not publish a partial catalog. |
| LI08 | Exact message/vocabulary lookup order | Custom selected locale, builtin selected locale, custom default locale, builtin default locale, then the stable key. The two namespaces remain distinct; lookup returns an output-owned string rather than a retained view into a foreign Context. |
| LI09 | Every checker key, value formatting and missing operand | Preserve the eight rule-to-message keys, translated location and separate system/input arguments. Absent operands render `-`; Null and TypedNull render `null`, timestamp renders milliseconds, and object/list values render their source markers. Scalar, JSON, date and decimal representations retain the existing Value formatting contract. |
| LI10 | Unicode input length, title casing and replacement order | Text length counts Unicode scalar values, not bytes or grapheme clusters. Both title helpers split paths and preserve index suffixes while applying the source Unicode uppercase/lowercase behavior. Four sequential replacement passes allow later keys introduced by earlier values; replacement is not recursively repeated for the same key. |
| LI11 | Output/domain copies and actual Context lifecycle | The unchanged default-PMR-disabled C++ test checks nested Global copies, retained catalog/errors/messages after actual User deletion, explicit output allocator identity, managed-object requirements, foreign/expired borrow rejection, catalog replacement, owner-thread mutation, module language precedence and terminal close. Existing empty or nonempty CheckResult messages are preserved by Context translation; the catalog's direct formatter and Context's fill-only-missing behavior remain separate. |
| LI12 | Original tests, full observations and honest frontend evidence | Execute all two language and two catalog tests from complete original modules. Replay all 4,575 existing language/catalog/Context observations and 60 full Debug/copy observations. The complete unchanged lifetime body is now permanently registered through imports; current native4/imported4 include generated vocabulary installation, alongside exact original/native/imported observations. Full native P7 integration remains independent. |

## Current execution and provenance

The [source/imported replay](parity/i18n_language_source_replay_5_0_6.json.gz)
retains all outputs, source/corpus/executable identities, exact reference registry
versions/checksums and the development wrapper. The existing two original
observers link the actual frozen runtime through the unchanged state-baseline
workspace. All outputs match their immutable active-baseline corpora. The two
registered imported CTests pass **2/2** after their build exits successfully.

The separate original unit-test crate imports the complete unchanged language
and catalog modules and their source-relative builtin/locale fixtures. Its
CheckResult, CheckRule, ObjectLocation, RuntimeError and Value dependencies are
the actual locked runtime/core types. Neither parser, translator nor enum is
reimplemented in the wrapper. Its full manifest, lock, wrapper and **4/4** test
result are retained in the replay receipt. This is development verification;
applications acquire no Rust runtime dependency.

The [imported lifetime receipt](parity/i18n_imported_source_5_0_6.json.gz) uses
the repository's unchanged `prepare_contracts.py` with a temporary in-memory
contract selection. Only that configuration read is substituted. The committed
contracts/presets remain unchanged. The normal canonical-body comparison proves
the entire original C++ body is preserved after replacing project includes with
imports. The actual pinned module compile command/map and completed module
archives compile and link the temporary consumer before execution.

This closed an evidence gap without changing inputs during the then-running native
epoch: the complete I18n lifetime body was not a committed imported registration.
Add its canonical registration after the epoch terminates, then reconcile the
source dispositions and current native outcomes. Do not mutate the frozen
configuration merely to turn this provisional review into a completed status.

## Existing adaptations and remaining acceptance

Catalogs and all output strings use explicit Context allocators. Rust shared
fallback ownership is represented by immutable builtin data and explicit copies,
not another ownership lifetime. Borrowed catalogs do not extend a Context.
Send/Sync is subject to the agreed C++ Context owner-thread contract.

The existing JSON-value decoder implements the supported JSON contract; this
review does not add a general Serde framework or non-JSON format guarantee.
Invalid C++ enum casts or ill-formed byte sequences are not fabricated Rust
Language/String inputs. Original source and C++ model boundaries remain explicit.

The [historical language/catalog acceptance](i18n_api_contract.md) keeps its old
5.0.5 source and sanitizer/MIPS scopes. Current source tests and imported
observations above are new evidence. No new sanitizer, MIPS, external translation
service or whole Context/Checker acceptance is claimed. G12/G13/G17 and P0-P7
retain their independent requirements, including R08's unanswered wire-profile
decision and [full native integration](native_integration_5_0_6.md).

## Installed source qualification

The complete existing I18n lifetime test is now a permanent canonical imported
contract and part of the module preset. The
[installed gates](parity/i18n_installed_gates_5_0_6.json.gz) pass **native 4/4**
and **imported 4/4**: the full ownership/Context body, both existing corpus
comparators and generated vocabulary/Checker installation. Canonical source
preparation preserves every original implementation and assertion body. All
892 current module consumer/fixture bodies verify; this count is structural
coverage, separate from the four executed imported tests.

The [source-unit project](../tests/parity/rust_i18n_source_5_0_6/Cargo.toml)
retains the earlier complete original-module wrapper with repository-relative
paths. Language, catalog and their source-relative builtin/locale JSON fixtures
remain actual frozen source files. CheckResult, CheckRule, ObjectLocation,
RuntimeError and Value remain actual runtime/core types. The two unchanged
observers use the existing reference-locked state-baseline workspace. Registry
versions/checksums and local package versions in both locks are verified before
compilation, as are the original source/observer/helper/corpus identities.

The [repository replay helper](../scripts/verify_i18n_source_5_0_6.py) runs all
**four original tests** and compares all **4,635/4,635 complete observations per
frontend** with fresh original results and retained expectations. Its
[terminal replay](parity/i18n_installed_replay_5_0_6.json.gz) stores commands,
outputs, dependency provenance and executable identities. SQL, errors, Unicode
strings, JSON framing and result payloads are not normalized. Existing corpora,
original source modules, C++ implementation/contract bodies and generated SDKs
remain byte-identical.

This accepts only the complete 215-line Language and 342-line I18n source files
under LI01-LI12 and the documented explicit Context ownership/threading
adaptations. The generated integration gate complements G12 R17; it does not
accept all model assembly, Checker/Context callers, R08, G13/G17 or P0-P7.
The historical failed full native epoch remains unchanged. No new full suite,
sanitizer, MIPS, external translation service or physical-device result is claimed.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_i18n_tests teaql_i18n_probe \
    teaql_i18n_debug_scope_probe teaql_generated_i18n_tests
cmake --build --preset modules_native --target teaql_module_i18n_tests \
    teaql_module_i18n_probe teaql_module_i18n_debug_scope_probe \
    teaql_module_generated_i18n_tests
ctest --preset native -R '^(i18n_context_contracts|i18n_rust_parity|i18n_debug_scope_rust_parity|generated_i18n_contracts)$'
ctest --preset modules_native -R '^module_(i18n_context_contracts|i18n_rust_parity|i18n_debug_scope_rust_parity|generated_i18n_contracts)$'
python3 scripts/verify_i18n_source_5_0_6.py \
    --module-build-dir build_modules_native --record /tmp/i18n_source_fresh.json.gz
```

The [terminal acceptance receipt](parity/i18n_language_source_acceptance_5_0_6.json.gz)
joins this complete review, installed gates, durable actual-source reproduction
and preserved before-state. All seventeen capability statuses remain unchanged.
