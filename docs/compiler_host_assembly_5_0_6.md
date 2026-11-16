# Compiler generation through trusted host assembly, Rust 5.0.6

Producer implementation:
[`90dbb3cf`](https://github.com/teaql/teaql-code-gen/commit/90dbb3cfc428df3728c6112bebd6e2aad49fa3c6).

## Confirmed entry-point gap

The existing compiler source service has complete model transport, compiled
metadata and business-consumer checkpoints. Its trusted map constructor is used
by tests and explicit assembly. The normal Spring `GenerateController`, however,
has two unannotated constructors and creates an empty service in its default
constructor. No production configuration registers the compiler capability.
Consequently, supplying compiler tools to the host cannot activate that path.

## Implementation and qualification plan

1. Register one trusted startup configuration that creates the ordinary
   DomainGenerationService. Four explicit tool/capability/job-directory settings
   install the existing CppCompilerGeneration for `cpp-lib-core`. Validate the
   complete configuration during startup and retain the existing bounded
   deadline. Inject this service into the ordinary controller.
2. Keep source snapshots, compiler/version validation, SDK verification,
   diagnostics and cleanup in their existing owners. Reuse the existing report
   callback through an optional trusted host bean. Add no C++ runtime dependency.
3. Exercise actual Spring assembly and controller uploads with valid, invalid,
   partial and changed capabilities. Compile the exact preceding controller
   separately and require the new upload regression to fail there.
4. Run complete original/C++ compiler jobs through this host and use their exact
   SDKs in unchanged typed SQLite/E/JSON and Business ID consumers. Retain
   original compiler refusals distinctly from host or mapping failures.
5. Qualify source/receipt continuity under ordinary and optimized Python, verify
   retained SDK bytes, run affected native checks and publish the bounded result.
   Keep full G13/G14/G17 and P0-P7 exits open.

The implementation uses the generator's existing Spring dependencies. Its
[host configuration guide](https://github.com/teaql/teaql-code-gen/blob/cpp_support/docs/cpp_compiler_host.md)
describes the required properties, environment variables and report observer.
Unconfigured interpretation remains a separately qualified path; this change
does not remove it or narrow the outstanding embedded-Rust requirement.

## Executed host contracts

The [fresh Java receipt](parity/compiler_host_units_5_0_6.json.gz) compiles current
production sources and the selected tests and passes 272 generator contracts, including
seven new host tests. Actual Spring bean construction and controller upload
exercise property/environment installation, configuration failures during startup,
changed capability rejection, original refusal propagation and report delivery
after job cleanup. These transport tests use an explicit stub worker; they do
not establish Rust compilation or generated business behavior.

The exact preceding controller is separately compiled into a first-priority
class directory. With that controller and the same new host tests, six pass and
the configured-upload test fails: the old controller uses its unconfigured
service instead of the installed bean. The current controller passes all seven.
Both outcomes and their actual commands remain in the receipt. All owned unit
classes and temporary paths are removed, with stable before/after input frames.

This uses actual Spring bean construction and invokes controller upload
handling. It does not start or deploy an HTTP server.

## Actual compiler and business execution

The [host execution receipt](parity/compiler_host_assembly_5_0_6.json.gz)
contains three actual compiler jobs through the Spring-configured service:

| Model | Host entry and outcome | Complete original / C++ SDK files |
| --- | --- | ---: |
| Qualified integer scalar | Uploaded controller request; generated successfully | 19 / 39 |
| Strong Business IDs | Uploaded controller request; generated successfully | 25 / 53 |
| Incompatible unsigned scalar | Service bean; original compiler refusal and no C++ SDK | Original refusal / no candidate |

The jobs use the explicit v4 semantic-storage capability. The original model
libraries are freshly compiled. An independent original generation reproduces
every original SDK file used by each successful job. The native consumers use
the exact C++ SDK manifests returned by that job.

The unchanged scalar observer matches two complete original/native SQLite
outcomes: populated and empty query results, typed getters, E access, JSON and
SQL intent diagnostics. The unchanged Business ID consumers match 1,044 Checker,
96 E and 480 Q/SQLite/JSON observations. In total, **1,622 paired business
observations match**. The existing native allocation/governed Save contract also
passes; it is separate native evidence rather than another paired Rust Save run.
Only the generated-header include is redirected in the copied C++ Business ID
observers. Other retained headers still compile their existing branches; those
branches do not become additional executed cases in this checkpoint.

## Preserved final-frame failure and independent admission

All twenty-four recorded execution commands return their required outcomes,
including exit 2 for the original compiler refusal. The original top-level
invocation nevertheless exits 1 in its final input-freeze check. During that
run, one sentence in the producer's host guide changed from referring to any
compiler property to referring specifically to settings in its table. The
report's internal `complete` marker describes the completed execution commands;
it does not turn that final CLI exit into success.

The [document transition](parity/compiler_host_document_transition_5_0_6.json.gz)
preserves the failure and both exact source images. Before/after execution
frames differ only at that document hash. Every executable source, reference
and recorded native input remains identical. The
[source admission](../scripts/compiler_host_source_admission_5_0_6.py) validates
that exact one-sentence continuation independently, with all original business
results and the initial exit retained. It does not claim a second business run.

The reader continuation adds receipt checks and corruption controls while
preserving every executed model, command and comparison body. The preceding
narrow-integer module delegates only its four source-matching functions to the
new source graph; its earlier controls and kernel evidence remain intact.
Unknown source hashes and unregistered Java additions still reject.

## Retained artifacts and affected native regression

The [read-only retention check](parity/compiler_host_retention_5_0_6.json.gz)
verifies all eighty SDKs, eighty manifests and 3,560 code/Assist files byte for
byte against the preceding reproduction. All 1,822 captured C++/module source
inputs are also unchanged. This is source/artifact identity evidence, not a
fresh SDK regeneration or imported-module run.

The [affected native gate](parity/compiler_host_native_5_0_6.json.gz) passes
8/8 tests and required source fixtures with exact registry/JUnit membership,
zero failures/errors/skips and a final no-work build. Its complete source,
reference and runtime/compiler/SQLite input hashes remain stable. The original
host execution removes 470,594,419 bytes of owned output; the Java gate removes
1,959,343 bytes. Their temporary paths are absent and compact reports remain.

## Completed compatibility boundary

The [compatibility receipt](parity/compiler_host_compatibility_5_0_6.json.gz)
finishes all twelve commands with stable source/reference/native inputs and
removed owned output. Both ordinary and optimized Python accept the completed
execution and reject the following corruptions:

| Responsibility | Rejected controls per Python mode |
| --- | ---: |
| New host source admission | 14 |
| Actual host execution receipt | 21 |
| Retained narrow-integer source admission | 13 |
| Retained narrow-integer execution receipt | 18 |
| Retained Business ID source admission | 12 |
| Retained scalar source admission | 13 |
| Total | 91 |

This closes the bounded trusted-host assembly checkpoint and its source/receipt
compatibility exit. The original final-frame failure remains explicit; the
independent admission does not relabel that CLI exit or repeat the business
execution. Complete configuration mappings, E03/E04
decisions, governed caller joins, G13/G14/G17 and P0-P7 remain open. This change
adds no runtime dependency, compatibility exception or capability promotion.

The [final checks](parity/compiler_host_checks_5_0_6.json.gz) pass the pinned
C++ formatter write/check, locked upstream and module-path checks, and both Git
whitespace checks with unchanged execution inputs. The
[milestone](parity/compiler_host_milestone_5_0_6.json.gz) retains the complete
evidence identities and unchanged fourteen qualified / three partial verdicts.
All ten recorded disposable parent/job/publication paths are absent; measured
parent cleanup totals 472,553,762 bytes. Shared caches and retained artifacts
remain in place. No sanitizer, MIPS, physical-device or full current native
epoch is claimed.
