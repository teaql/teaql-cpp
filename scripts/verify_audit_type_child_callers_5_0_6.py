#!/usr/bin/env python3
"""Compile original entity and native privacy callers with the qualified type overlay."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CASES = ['type_child_paren_trailing_closure', 'type_child_optional_promoted_closure',
         'type_child_paren_dyn_promoted_closure', 'type_child_paren_optional_nested_closure',
         'type_child_paren_impl_promoted_closure', 'type_bound_dyn_double_paren_trait_closure',
         'type_bound_bare_paren_lifetime_closure', 'type_bound_dyn_generic_bound_call_closure',
         'type_bound_function_mut_named_self_closure', 'type_rule_dyn_earlier_generic_turbocall_closure',
         'type_child_path_closure', 'type_child_paren_promoted_closure']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--comparison', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and not args.comparison.exists()
    proposal_path = ROOT / 'docs/parity/audit_type_contract_proposal_5_0_6.json.gz'
    proposal = json.loads(gzip.decompress(proposal_path.read_bytes()))
    assert proposal['complete'] is True
    candidate = proposal['candidate_source']
    assert hashlib.sha256(candidate.encode()).hexdigest() == proposal['candidate_sha256']
    record = dict(complete=False, scope=__doc__, steps=[],
                  proposal_sha256=hashlib.sha256(proposal_path.read_bytes()).hexdigest())
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_audit_type_callers_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            source = owned / 'CppRustLiteralMetadata.java'
            source.write_text(candidate)
            corpus = ROOT.parent / 'teaql_code_gen_cpp/generator/src/test/resources/cpp_type_child_metadata_5_0_6.json.gz'
            command = ['python3', 'scripts/verify_audit_metadata_compiler_5_0_6.py',
                       '--candidate-parser', str(source), '--corpus', str(corpus),
                       '--cases', *CASES, '--record', str(args.comparison)]
            result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
            record['steps'].append(dict(command=command, exit_code=result.returncode,
                                        stdout=result.stdout, stderr=result.stderr))
            result.check_returncode()
            observed = json.loads(gzip.decompress(args.comparison.read_bytes()))
            assert observed['complete'] is True and len(observed['cases']) == len(CASES)
            record['comparison_sha256'] = hashlib.sha256(args.comparison.read_bytes()).hexdigest()
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Qualified twelve original/native generated descriptor and field-privacy pairs')


if __name__ == '__main__':
    main()
