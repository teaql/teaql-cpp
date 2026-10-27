"""Read explicitly approved exceptions without changing frozen source results."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'docs/compatibility_exceptions_5_0_6.json'
EVIDENCE = {
    'tests/parity/generated_relation_analysis_5_0_6_cases.json.gz',
    'docs/parity/aggregation_raw_metadata_bridge_results_5_0_6.json.gz',
    'docs/parity/aggregation_raw_metadata_bridge_source_5_0_6.json',
    'docs/parity/generated_member_alias_quoted_witness_5_0_6.json.gz',
    'docs/parity/generated_member_alias_quoted_preservation_5_0_6.json.gz',
    'docs/parity/generated_wire_mappings_profile_inspection_5_0_6.json.gz',
}


def approved_exception(name, manifest_path=MANIFEST):
    contract = json.loads(Path(manifest_path).read_text())
    runtime = json.loads((ROOT / 'upstream.lock.json').read_text())
    producer = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    if (contract.get('schema') != 1 or contract.get('runtime_commit') != runtime['commit']
            or contract.get('generator_commit') != producer['generator_commit']):
        raise RuntimeError('Compatibility exceptions use different locked references')
    exception = contract[name]
    if exception.get('status') != 'approved':
        raise RuntimeError(f'Compatibility exception is not approved: {name}')
    if not (ROOT / exception['documentation']).is_file():
        raise RuntimeError('Compatibility exception documentation is missing')
    if set(contract['evidence_sha256']) != EVIDENCE:
        raise RuntimeError('Compatibility exception evidence set is incomplete')
    if name == 'quoted_fk':
        indices = exception['exception_case_indices']
        if (exception['relationship_corpus'] != 'tests/parity/generated_relation_analysis_5_0_6_cases.json.gz'
                or exception['exact_relationship_cases'] != 6576
                or exception['exception_relationship_cases'] != 546
                or exception['direct_fk_cases'] != 12 or exception['direct_fk_differences'] != 5
                or len(indices) != 546 or any(type(i) is not int for i in indices)
                or indices != sorted(set(indices)) or indices[0] < 0 or indices[-1] >= 7122):
            raise RuntimeError('Quoted-FK exception exceeds its approved observation scope')
    for relative, expected in contract['evidence_sha256'].items():
        if hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() != expected:
            raise RuntimeError(f'Compatibility exception evidence changed: {relative}')
    return exception
