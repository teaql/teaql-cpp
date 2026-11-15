# Source/control observer overhead diagnostic

The first observed CTest selection is interrupted because the observer counts
every ordinary local function call, including millions of recursive JSON
comparisons. This is an instrumentation defect, not a runtime acceptance
failure. Test commands, source contracts and rejection conditions are unchanged.

Only the verified task-owned CTest process is signaled. Its coordinator retains
the terminal diagnostic and cleans its owned temporary directory.

Observer SHA-256: `b3d611cf2429cd80a1577365c8915a7cf3217eac34024df8a06df1fcb95ccb53`.

Completed process observations before interruption:

```json
[
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/tests/parity/verify_generator_child_checks_5_0_6.py",
      "--build-dir",
      "/home/philip/githome/teaql-cpp/build_native_current"
    ],
    "calls": 15600,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/scripts/model_driver_provenance_hardened_5_0_6.py",
      "verify",
      "--contracts",
      "/home/philip/githome/teaql-cpp/tests/parity/generated_model_driver_contracts_5_0_6.json",
      "--execution",
      "/home/philip/githome/teaql-cpp/docs/parity/generated_model_driver_execution_5_0_6.json.gz"
    ],
    "calls": 10143246,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/scripts/verify_sql_executor_driver_contracts_5_0_6.py",
      "--contracts",
      "/home/philip/githome/teaql-cpp/tests/parity/sql_executor_driver_contracts_5_0_6.json",
      "--execution",
      "/home/philip/githome/teaql-cpp/docs/parity/sql_executor_driver_execution_final_5_0_6.json.gz"
    ],
    "calls": 906532,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/tests/parity/verify_configuration_cli_recipes_5_0_6.py",
      "--ctest-build-dir",
      "/home/philip/githome/teaql-cpp/build_native_current"
    ],
    "calls": 94695,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/scripts/inherited_business_driver_provenance_5_0_6.py",
      "verify",
      "--contracts",
      "/home/philip/githome/teaql-cpp/tests/parity/inherited_business_driver_contracts_5_0_6.json",
      "--execution",
      "/home/philip/githome/teaql-cpp/docs/parity/inherited_business_driver_execution_5_0_6.json.gz"
    ],
    "calls": 88252,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/scripts/verify_upstream.py"
    ],
    "calls": 5,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/tests/parity/verify_composite_corpus_source_controls_hardened_5_0_6.py"
    ],
    "calls": 11797757,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/tests/parity/verify_private_runtime_driver_controls_hardened_5_0_6.py",
      "--contracts",
      "/home/philip/githome/teaql-cpp/tests/parity/private_runtime_driver_contracts_5_0_6.json",
      "--execution",
      "/home/philip/githome/teaql-cpp/docs/parity/private_runtime_driver_execution_5_0_6.json.gz"
    ],
    "calls": 18943745,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/scripts/generated_driver_provenance_hardened_5_0_6.py",
      "verify",
      "--contracts",
      "/home/philip/githome/teaql-cpp/tests/parity/core_model_driver_contracts_5_0_6.json",
      "--execution",
      "/home/philip/githome/teaql-cpp/docs/parity/core_model_driver_execution_5_0_6.json.gz"
    ],
    "calls": 7406953,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/tests/parity/verify_archive_driver_controls_5_0_6.py",
      "--contracts",
      "/home/philip/githome/teaql-cpp/tests/parity/sql_mutation_driver_contracts_5_0_6.json",
      "--execution",
      "/home/philip/githome/teaql-cpp/docs/parity/sql_mutation_driver_execution_5_0_6.json.gz"
    ],
    "calls": 7771687,
    "observer_errors": []
  },
  {
    "argv": [
      "/home/philip/githome/teaql-cpp/scripts/generated_driver_provenance_hardened_5_0_6.py",
      "verify",
      "--contracts",
      "/home/philip/githome/teaql-cpp/tests/parity/context_public_driver_contracts_5_0_6.json",
      "--execution",
      "/home/philip/githome/teaql-cpp/docs/parity/context_public_driver_execution_5_0_6.json.gz"
    ],
    "calls": 4525258,
    "observer_errors": []
  }
]
```

## Subsequent function-tracing limit

After removing the C-call profiler, the ordinary continuation still reaches the
600-second CTest limit for `parity_dynamic_json_cli_source_controls`. That
registration emits no verifier exception before termination. Its historical
source record occupies 6,575,659 compressed bytes and 471,099,679 uncompressed
bytes; the eleven retained raw result streams contain 119,399,084 characters.

An actual same-command run without the startup observer passes in
318.61774157499894 seconds and rejects all 46 source/profile/raw-stream/CTest
substitutions. Its owned temporary directory is removed. This establishes an
observation-overhead failure for that comparison, while leaving the registered
timeout and all acceptance predicates unchanged.

The isolated observer candidate therefore adds an explicit `audit_only` mode.
It preserves startup argument/mode identity, execution-code hashes, explicit
dynamic compilation hashes and actual subprocess audit events. It leaves both
the Python trace hook and C-call profiler unchanged and declares an empty
function-call observation list. Function and assertion-line observations remain
available through the separate `calls` mode.

All fourteen existing call-mode fixtures and six new audit-mode fixtures pass:
ordinary/optimized valid and invalid dynamic code, and two threaded-spawn cases
with eight real children each. Candidate output cleanup removes 59,163 bytes.
An initial candidate check incorrectly expected CPython to emit a source
`compile` event for the main script; the minimal diagnostic shows an `exec`
event instead. The corrected fixtures explicitly compile dynamic source and
check both that compilation and its execution.

The candidate is prepared outside project source while the ordinary collection
keeps its inputs frozen. After that collection terminates, its source is
activated byte for byte. Installed ordinary and optimized preflights each pass
twenty observer cases and four inventory corruptions. Root protocol controls
accept four actual trace shapes and reject 68 corruptions per controller mode.
The [disposition and continuation record](source_control_dispositions_5_0_6.md#current-continuation-and-reuse)
retains the incomplete original collection and exact current-command reuse.
Audit-only evidence keeps its declared scope when combined with earlier focused
function traces; the terminal ordinary and optimized selections remain open.
