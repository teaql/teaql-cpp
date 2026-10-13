# Locked generator parity model inputs

These XML inputs are copied byte for byte from the locked Rust 5.0.6 release
or its recorded generator producer. provenance.json records their origin and
hashes. Authentic model identifiers are preserved; project paths use underscores.
They cover shared load state/checker/constants, general queries/aggregates,
human/non-human predicate naming and JSON aliases.

They are reference inputs, not a claim that the current C++ generator supports
all their shapes. The existing default examples/type fixtures remain runnable
baselines. Add actual generated C++ execution/source comparisons as each phase
qualifies its runtime dependencies. Inheritance/wide extensions must come from
the producer's original fixture construction and retain explicit provenance.
