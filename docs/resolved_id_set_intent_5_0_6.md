# Resolved ID-set execution with explicit intent (Rust 5.0.6)

## Missing evidence

The existing 233-input `id_set_page_5_0_6_cases.json.gz` faithfully records
historical inputs. They carry a comment but no purpose. Exactly 108 inputs reach
private ID-set construction and are rejected by prepared-query intent validation
before any physical ID query. Those outcomes remain valid rejection evidence;
they cannot establish the later provider, row-validation, write or hit paths.

This continuation retains the exact historical 233 inputs and adds 233 inputs
with `purpose = "Verify retained ID-set execution"`. It does not change public
intent requirements, runtime ownership, provider scope or generated APIs.

## Source and candidate

The observer is appended to the clean locked Rust 5.0.6 `resolved.rs`. It invokes
actual `prepare_id_set_page` and its unchanged helpers, with the original package
registry versions. The existing source driver supplies recording stores and a
provider; the only semantic input extension is optional purpose. Recorded
provider requests are controlled executor calls, not live SQLite statements. The historical
archive bridge is replaced by direct private access within the appended test
module. All original runtime bodies remain byte-identical.

The existing native probe gains the same optional input, so its old corpus is
still run unchanged. The comparator rejects changed historical input hashes,
changed original-source/observer hashes, changed historical results, incomplete
observation sets, a changed executable and any result difference. Native and
module consumers use the same fixture implementation. The ID-set page probe had
no canonical module target; the initial module build reported an unknown target.
This continuation registers that existing fixture in the module manifest and
preset before compiling its imported form.

The initial observer launch was stopped after detecting an incorrect test filter
before obtaining behavior evidence. Its owned child process was terminated and
reaped and its temporary source/build tree removed. The corrected run is recorded
separately; a cancelled launch cannot qualify a source behavior.

## Responsibility reconciliation

| Locked source branch | C++ responsibility and evidence |
| --- | --- |
| Disabled option; missing slice/limit; continuous, partition, aggregate, group, HAVING, raw SQL or expression sort | `IdSetPageOperation::Prepare` preserves each fallback guard before store access. The retained shape matrix includes all listed unsupported kinds. |
| Deterministic tie break and scoped key | Add ascending `id` only when no plain `id` sort exists. Descending ID and non-ID tie sorts retain distinct inputs. Exact key text is compared; separate key/source qualification covers user/root/purpose normalization. |
| First get, build lock, second get | Both read failures remain fail-open. Repeated calls test build then hit; existing `id_set_page_contracts` separately tests a suspended waiter rechecking after acquiring the lock, avoiding duplicate I/O. |
| ID provider request | Clear projection/enhancement presentation, select `id`, disable paging options and bound offset/limit. Compare complete physical query Debug and provider errors, including capture modes. |
| ID validation, maximum and write | Preserve missing/text ID fallback, over-limit observation and put failure. Fixed small bounds exercise exact and excessive row counts. Saturating `max_ids + 1` and checked expiry retain source-equivalent implementation branches; this corpus does not execute extreme TTL overflow. |
| Retained page projection | Preserve zero-sized and out-of-range pages, `UINT64_MAX` offset, total cardinality, cleared optimization and added ID filter. The final query is compared in full. This native 64-bit observation is not new 32-bit ABI evidence. |
| C++ ownership adaptation | Existing build/page contracts cover cancellation, actual Context close, released locks, checked store borrows and Global isolation. No ownership implementation changes in this continuation. |

## Verification

- [Original observer](parity/resolved_id_set_intent_original_v2_5_0_6.json.gz): 466 results from complete unchanged runtime bodies.
- [Native](parity/resolved_id_set_intent_native_5_0_6.json.gz) and
  [module](parity/resolved_id_set_intent_modules_5_0_6.json.gz): 466/466 match each.
- Native CTest: `resolved_id_set_intent_rust_parity`, `id_set_page_rust_parity`,
  `id_set_page_contracts` and `id_set_build_contracts`, 4/4 passed.
- Canonical consumer verification: 1,132 bodies. Pinned formatting/check:
  1,796 files. Locked-source inventory, module paths and whitespace checks pass.

The [checkpoint](parity/resolved_id_set_intent_checkpoint_5_0_6.json.gz) retains
JUnit, the test log, receipt hashes and cleanup observations. The old inputs
make zero provider calls and zero puts, with 208 intent errors across 108 inputs.
The explicit-purpose inputs make 153 provider calls and attempt 61 puts; eight
provider errors propagate. Final plans include 55 hits, two builds, 18 limit
fallbacks, seven store-unavailable fallbacks, 116 unsupported-shape fallbacks and
14 disabled outcomes. Twenty-one root-control inputs in each half have their
own output shape. These counts describe the retained inputs, not general rates.

Both task-owned Rust views/builds were removed: 37,773,845 bytes from the cancelled
launch and 161,167,809 bytes from the completed run. Stable native/module preset
outputs remain available. No new sanitizer, MIPS, live external service or
physical-router result is claimed. Production and generator bodies are unchanged.

Complete resolved-source and capability verdicts remain unchanged. This closes
the missing provider-reaching evidence for the retained RQ12 matrix, not every
pagination caller, scheduling interleaving, query configuration or P0-P7.
