# Private memory stable ordering: Rust 5.0.6

Status: focused native/module/sanitizer qualification.
Whole private-source acceptance remains pending.

## Implementation and contract

The private C++ memory-service fixture previously used stable insertion sort to
avoid the hidden heap scratch allocation of `std::stable_sort`. That implementation
had quadratic worst-case comparison cost. It now uses a bottom-up stable merge
sort with one reusable scratch vector allocated from the input vector's Context.
Each merge selects the left item on equality, preserving stable order. Width and
range calculations avoid unsigned overflow. Neither a default PMR resource nor
an additional ownership domain holds the scratch data.

Each pass moves O(n) items and uses at most n comparisons; at most
ceil(log2(n)) passes give O(n log n) comparisons and moves. The scratch vector
holds O(n) items. This is not a claim that all query allocations are O(n): the
unchanged expression comparator can allocate Context-owned temporary Values.
Multiple order keys still execute as reversed stable passes, matching the source.
Field absence, expression-error handling and comparison rules are unchanged.

Rust `sort_by` requires a total-order comparator. The original private comparator
can map incomparable mixed Values to equality, violating that precondition.
The [retained ordering-contract review](memory_source_upgrade_audit_5_0_6.md#ordering-contract-boundary)
therefore remains applicable: these tests qualify valid ordering inputs and do
not promise a particular permutation for arbitrary non-total inputs.

## Evidence

The [original observation](parity/memory_sort_original_5_0_6.json.gz) calls
byte-preserved private Rust bodies for 60 cases: sizes 0, 1, 2, 17 and 257;
ascending, reverse and duplicate-key distributions; ascending, descending,
two-key and GBK expression ordering. Complete rows, including original IDs and
secondary values, are compared. The [native comparison](parity/memory_sort_native_5_0_6.json.gz)
matches 60/60, as do [modules](parity/memory_sort_modules_5_0_6.json.gz) and
[ASan/UBSan](parity/memory_sort_sanitize_5_0_6.json.gz). The
[execution checkpoint](parity/memory_sort_checkpoint_5_0_6.json.gz) retains native
3/3 and sanitizer 2/2 CTests, module complexity output, 1,117 canonical body checks
and source/path verification. Pinned formatting and checks pass for 1,785
handwritten C++ files; affected builds finish with no work remaining. Sanitizer leak scanning is disabled and undefined
behavior stops execution. Existing pipeline comparison also passes, including missing keys,
expression errors and sorting before slicing/projection.

A separate C++ complexity contract exercises six sizes through 8,193 elements
and three distributions. It checks sorted order, stable equal-key IDs, complete
unique item retention, allocator identity and the n*ceil(log2(n)) comparison
bound. The largest duplicate-key case uses 98,779 comparisons against a 114,702
bound. This is a deterministic algorithm check, not a timing benchmark or a Rust
comparison-count equivalence claim. All probes disable the default PMR resource.

## Reproduction and scope

```bash
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_memory_sort_probe teaql_memory_pipeline_probe
ctest --test-dir build_native_current \
  -R '^(private_memory_sort_.*|private_memory_pipeline_rust_parity)$' --output-on-failure
```

The original runner removed 160,973,716 bytes of owned runtime-view/build outputs
after the child exited. Stable preset build directories are reused; source and
compact receipts are retained. Production runtime and generated artifacts are
unchanged. No MIPS or physical-device execution is claimed. Recursive evaluator
review and complete source consolidation remain open.
