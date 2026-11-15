# Reverse-relation module paths against Rust 5.0.6

This bounded AP02 diagnostic uses runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. It identifies a generated-consumer
defect; it does not qualify the remaining AP02 surface.

Freshly compiled original and C++ Java generators consume the same three
models. Each original generated Rust library is compiled with dependency
identities checked against the locked runtime, and its small consumer runs.
Each genuine C++ generated header is checked with the configured native
compiler. Generated files remain unchanged throughout the probe.

| Reverse-relation `json_type` | Original Rust build/run | C++ header compilation |
| --- | --- | --- |
| `Self` | Pass | Pass |
| `super::Item` | Pass | Fail: undeclared `super` |
| `crate::item::Item` | Pass | Fail: undeclared `crate` |

The original entity lives in `item::entity`; its parent module re-exports
`Item`, and the crate exposes the `item` module. These two paths therefore
refer to the same nominal entity as `Self`. The C++ generator currently writes
the Rust paths directly into `projection.reverse_one<...>`. Metadata parsing
success alone does not establish a usable generated adapter.

The diagnostic preserves 11,968 source inputs and both reference checkouts.
All compiler processes terminate before their results are inspected. The
272,475,776-byte owned build directory is removed, retaining a compressed
command/source/diagnostic receipt. The audit has terminated. The
[retained receipt index](parity/ap02_reverse_relation_nominal_type_candidate_index_5_0_6.json.gz)
maps every original temporary receipt path to its unchanged repository copy.

The implementation sequence is:

1. Finish the current source-control audit and preserve its exact source epoch.
2. Resolve nominal model paths according to the original generated module
   layout, using the existing centralized Rust module/type naming. Keep
   inaccessible, missing and non-Entity types rejected. Reuse the runtime's
   existing typed projection implementation.
3. Compile fresh original and C++ generated consumers for public paths and
   invalid boundaries. Exercise affected relation/E/JSON behavior, reproduce
   affected SDKs and examples, and retain source-bound results. Generated
   artifacts are regenerated rather than edited.

No runtime function, ownership domain, module export or generator source has
changed at this diagnostic checkpoint. This is a C++ generator defect and
does not need a Rust compatibility exception.

## Candidate validation

A second original/C++ compiler diagnostic adds seventeen cases. Together the
two diagnostics cover twenty distinct type spellings: fourteen original Rust
libraries compile and run, and six fail at the original type/visibility
boundary. Current C++ generation accepts three of the fourteen valid cases;
eleven valid original consumers fail C++ compilation. The second owned build
is removed, reclaiming 458,927,910 bytes.

The valid cases include root re-exports, public model modules, the owner's
private `entity` module, parenthesized nominal types and empty type arguments.
The invalid cases retain unimported sibling names, another model's private
`entity` module, an excessive parent path and a tuple that does not implement
`Entity`. Original dependency identities remain locked for every build.

A copied Java candidate adds a nominal-path reader and maps model types using
the original generated module visibility. It preserves the general metadata
parser and delegates typed projections to the existing runtime. Fresh candidate
generation and actual C++ compilation now match all twenty original outcomes:
**14 accept / 6 reject, zero differences**. This is candidate-only evidence;
the producer checkout and retained generated artifacts remain unchanged.

Install the reviewed candidate after the source-control checkpoint. Permanent
generator regression cases and production source/artifact requalification
remain required before calling the repair qualified. The twenty-case diagnostic
does not replace full AP02 or G14 exits.

A further candidate-only probe freshly generates and compiles original Rust
and C++ models for `super::Item`, `crate::child::Child` and
`((super::super::child::Child))`. Using the unchanged retained SQL fixture and
consumer harnesses, all **15/15 query results** match across three models.
They exercise actual SQLite relation loading, E and JSON for unselected,
loaded, empty, minimal-projection and missing-parent requests. Generated C++
uses the pinned formatter, original dependencies remain locked, and all
11,968 project inputs remain unchanged. Candidate compilation and behavior
are established in this finite scope; installation, production generation,
permanent regression cases and source/artifact requalification remain open.

The initial candidate unnecessarily rejected empty arguments on `Self`. A
[language-only diagnostic](parity/ap02_reverse_relation_self_arguments_language_5_0_6.json.gz)
disproves that expectation; its `complete: false` remains intact. The
[final copied candidate](parity/ap02_reverse_relation_nominal_type_candidate_complete_5_0_6.json.gz)
removes that exclusion. Two fresh original generated libraries using `Self<>`
and `Self::<>` compile and run; baseline C++ rejects them, and the final
candidate compiles both. It also reproduces all 53 files of each preceding
twenty compiler SDKs and three formatted SQLite SDKs without a difference.
Their actual earlier compiler/query results are reused explicitly.

The final finite scope is **22 type spellings: 16 accept / 6 reject**, plus
the fifteen actual SQLite query outcomes. All six compact receipts are retained
unchanged; 1,391,348,632 bytes of owned builds and all temporary receipt copies
are removed. The copied final source is ready for installation and permanent
regressions; the producer checkout remains unchanged at this checkpoint.
