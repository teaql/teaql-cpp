# Early SQLite adapter retirement

Source checkpoint: changes after tag `0.0.2`, commit
`63e4d50526e0525aa02d92e6367b49237b26796e`.

The user requested removal of the extra early C++ SQLite adapter. The eight
package files, forwarding includes, library target and test link dependencies
are removed. `sqlite_contracts` retains its CTest name and uses a new regression
source over the real runtime, SQL executor and SQLite provider. The source
algorithms, module interfaces, ownership contracts and Rust v5.0.5 baseline
are unchanged. See [the coverage disposition](../../../docs/sqlite_compatibility.md).

| Validation | Result |
| --- | --- |
| Native Clang23.1.3/libstdc++16.2, Debug | Full 388/388 pass in 91.01 seconds, including provider regressions, source comparisons and both examples. |
| Native named-module preset | 72/72 imported checks pass in 1.20 seconds. |
| Targeted ASan/UBSan | 2/2 changed consumers pass in 0.04 seconds after a successful fresh header-profile build; leak scanning disabled. |
| Current target graphs | Header and module Ninja graphs have no legacy target or compiler inputs. Old ignored build artifacts are not active targets. |
| Rust acceptance records | 82 source files, 330 declarations, 451 tests and 58 responsibility groups remain; all 441 in-scope dispositions are covered, with five backend and five external exclusions. All named checks remain registered. |
| Production checkpoints | 309 retained original production files remain unchanged; one earlier structural relocation and seven specifically retired move records are accounted for. Original move hashes are preserved. |
| Public interfaces | Source root exports, all 1,284 named-module catalog exports, 75 canonical consumers and the acyclic 208-header/552-edge graph pass. |
| Runtime boundary | Compiled runtime contains no SQLite C API calls; independent kernel dependencies pass. |
| Formatting | 700 handwritten C++ files pass pinned clang-format18.1.8; all control-flow bodies use braces. |

Native and module selections may overlap; their counts are not summed into a
new source-test coverage claim. This is a maintenance regression, not a new
Rust parity or native module-ownership milestone. No MIPS/QEMU repetition,
physical device test, GPU adapter or generator modification is included.

Raw native CTest output and registration are retained, as is the completed
module CTest execution log. `active_graphs.json` records inspection of the
configured Ninja target lists and compile command inputs. The sanitizer
selection covers the two changed executable consumers and keeps leak scanning
disabled. The previous module release's full sanitizer/MIPS results remain
historical, not executions of this maintenance snapshot.
