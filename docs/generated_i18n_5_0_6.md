# Generated application internationalization

This G12 checkpoint restores the application vocabulary catalog and its explicit
Context installation workflow. It retains the complete remaining assembly,
sample-data and Assist requirements; G12 and P0–P7 are not declared complete.
References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Restored responsibility

The locked Rust I18n Assist requires an application catalog, but the C++ SDK
previously emitted neither that file nor installation guidance. The existing
C++ runtime already implements catalog parsing, lookup, Context language,
translation and checked ownership. No runtime algorithm needs replacement.

The C++ generator now delegates catalog creation to the existing
`JavaWorkspaceI18nSampleGenerator.generateCatalog`. This is the same unchanged
helper called by `RustWorkspaceI18nGenerator`, with the same `EntityDomain`
wrapper. No dependency or duplicate catalog-generation algorithm is introduced.

| Source responsibility | C++ disposition |
| --- | --- |
| Model vocabulary | Preserve the shared generator's entity/property keys and title conversion, including authentic model names and keyword fields. These keys are not C++ escaped accessor names. |
| Locale entries | Retain all fifteen canonical locales, English default vocabulary, modeled Simplified Chinese vocabulary and empty message maps from the source. Translations supplied by a model remain fixture/application data. |
| Catalog wire format | Emit the same `teaql.i18n/v1` JSON and authentic external filename `teaql-i18n.json`. This established filename is not a C++ package/module naming change. |
| Installation | Application code reads the file explicitly, parses through `I18nCatalog::from_json`, checks the Result, allocates a managed catalog and installs its checked reference on `user_context`. Neither generation nor module registration performs runtime file I/O. |
| Language selection | Use runtime `set_locale_code`; an unsupported code returns typed `UnsupportedLocale` and leaves the previous language unchanged. Failed parsing does not install a replacement. |
| Lookup | Application selected locale, builtin selected locale, application default locale, builtin default locale, then the stable key. Empty generated message maps retain builtin messages. |
| Checker translation | Translate display messages while retaining typed rule IDs and already supplied messages. Vocabulary lookup is separate; this does not claim automatic replacement of every Checker location with a modeled label. |
| Ownership | Parsed wrappers must end before their allocator. Installed catalogs are checked Context borrows. Retention uses an explicit Global copy and rebind through UserContext. |
| Guidance | Generated Assist documents the actual kernel spellings. Its final camelCase pass preserves `from_json`, `make_ref`, `set_i18n_catalog` and `set_locale_code`. |

The catalog ships alongside the SDK's existing development assets. Applications
choose how to read/package it and customize translations. It is not loaded from
the process environment or implicitly installed by `setup`.

## Verification

The [pre-change snapshot](parity/generated_i18n_before_5_0_6.json.gz) preserves
complete locked source bodies, prior producer/scripts/ledger and all 77 SDK file
sets. The producer regression fails on the old missing file; the repaired
[Java gate](parity/generated_i18n_java_5_0_6.json.gz) passes **72/72** tests.

The [generation proof](parity/generated_i18n_regeneration_5_0_6.json.gz) invokes
the actual frozen Rust workspace catalog generator for every successful SDK.
All **63** complete catalog files match byte-for-byte. Strict reproduction covers
**24 normal workspaces / 312 files**, **50 contract destinations** and **three
identity fixtures**. Every pre-existing C++ code byte is preserved. The new file,
installation guidance, source fingerprints and artifact-count metadata account
for the changes; rejected-model fixtures retain their original semantic errors.

The initial recording script incorrectly assumed that all probe envelopes used
`file_count`. Candidate/constant probes instead report the numeric `files` count.
Both failed attempts are retained in
[the initial record](parity/generated_i18n_regeneration_initial_attempt_5_0_6.json.gz)
and [the schema follow-up](parity/generated_i18n_regeneration_observer_schema_attempt_5_0_6.json.gz).
The continuation verifies the completed 25-SDK prefix and permits exactly one
extra reported file using each observer's actual key. All other observation
fields must remain equal. Runtime outputs are not normalized.

The [runtime corpus](../tests/parity/generated_i18n_5_0_6_cases.json.gz) contains
**82** actual original/native observations: all 63 catalogs, fifteen selected
locales and four Context controls. They compare complete vocabulary/message
outputs, invalid locale errors, canonical aliases, explicit existing messages
and application message overrides. Asset fingerprints are checked before replay.
The [observation record](parity/generated_i18n_runtime_observations_5_0_6.json.gz)
retains the full inputs, both outputs and executable identities.

The [generated integration test](../tests/generator/generated_i18n_tests.cpp)
opens and installs the real generated catalog, runs a generated Checker with
application logic, selects custom messages without changing rule IDs, preserves
language after failure and uses an explicit Global catalog after actual source
UserContext destruction. Existing runtime and generated Checker controls remain
part of the native selection. Production runtime bodies remain unchanged.
The final selection passes **5/5** after the resource-scoped file reader is
rebuilt. Both formatting and formatting verification pass for **1,642**
handwritten C++ files.

Reproduce with the locked local toolchain:

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_generated_i18n_tests teaql_generated_checker_tests \
  teaql_i18n_tests teaql_i18n_probe
ctest --preset native --output-on-failure \
  -R '^(generated_i18n_contracts|generated_i18n_rust_parity|generated_checker_contracts|i18n_context_contracts|i18n_rust_parity)$'
```

The [terminal record](parity/generated_i18n_terminal_5_0_6.json.gz) reconciles
source, generation, native and formatting evidence. Existing sanitizer/module/
MIPS results retain their historical scope; this data/guidance change adds no
production lifetime algorithm or public C++ declaration.
