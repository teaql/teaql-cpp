#!/usr/bin/env python3
"""Validate complete source/profile corpora and their current native observers."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from custom_public_cli_source_provenance_5_0_6 import digest, read, require


def lock_identities(text):
    return {
        tuple(re.search(rf'^{key} = "([^"]+)"', block, re.M)[1]
              for key in ('name', 'version', 'source', 'checksum'))
        for block in text.split('[[package]]')[1:]
        if '\nsource = "registry+' in block
    }


class Validator:
    def __init__(self, root, registry):
        self.root = root.resolve()
        self.registry = registry
        sys.path.insert(0, str(self.root / 'tests/parity'))
        from compare_values import json_lines, same
        self.lines, self.same = json_lines, same

    def load(self):
        entry = self.registry['source_execution']
        require(digest(self.root / entry['path']) == entry['sha256'], 'source artifact identity')
        self.source = read(self.root / entry['path'])
        return self

    def validate_files(self):
        r = self.registry
        require(json.loads((self.root / 'upstream.lock.json').read_text())['commit']
                == r['runtime_commit'], 'current runtime baseline')
        for name, commit in [('teaql-rs', r['runtime_commit']),
                             ('teaql_code_gen', r['generator_commit'])]:
            reference = self.root / '.reference' / name
            require(subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'],
                                            text=True).strip() == commit, 'reference identity')
        for relative, wanted in r['support_files'].items():
            require(digest(self.root / relative) == wanted, 'observer/helper identity: ' + relative)
        for relative, wanted in self.source['source_files'].items():
            require(digest(self.root / relative) == wanted, 'original observer identity: ' + relative)
        import archive_driver_provenance_5_0_6 as archive
        data = subprocess.check_output(['git', '-C', str(self.root / '.reference/teaql-rs'),
                                        'archive', r['runtime_commit']])
        require(hashlib.sha256(data).hexdigest() == self.source['archive_sha256'], 'source archive')
        require({k: hashlib.sha256(v).hexdigest() for k, v in archive.archive_files(data).items()}
                == self.source['archive_files'], 'complete archived source members')

    def selected_cases(self, contract):
        require(digest(self.root / contract['corpus']) == contract['corpus_sha256'], 'corpus identity')
        corpus = read(self.root / contract['corpus'])
        require(corpus['baseline'] == self.registry['runtime_commit'], 'corpus baseline')
        rows = corpus['cases']
        require(type(rows) is list and rows, 'nonempty original corpus')
        if contract['group'] != 'all':
            require({c['probe'] for c in rows} == {'audit', 'sql', 'config'}, 'complete formatter groups')
            rows = [c for c in rows if c['probe'] == contract['group']]
        require(rows, 'nonempty original group')
        return rows

    def validate_source(self, source):
        r = self.registry
        require(source['complete'] is True and source['whole_goal_complete'] is False,
                'source execution scope')
        require(source['runtime_commit'] == r['runtime_commit']
                and source['generator_commit'] == r['generator_commit'], 'source baseline')
        require(source['input_hashes_before'] == source['input_hashes_after']
                and source['references_before'] == source['references_after'], 'source execution epoch')
        require(self.same(source['source_files'], self.source['source_files']), 'copied original inputs')
        project = source['derived_observer']
        require(digest(self.root / '.reference/teaql-rs/Cargo.lock') == project['seed_lock_sha256'],
                'reference lock seed')
        require(hashlib.sha256(project['manifest'].encode()).hexdigest() == r['manifest_sha256']
                and hashlib.sha256(project['compiled_lock'].encode()).hexdigest() == r['compiled_lock_sha256'],
                'derived original project')
        require(project['compiled_lock_sha256'] == r['compiled_lock_sha256'], 'compiled lock identity')
        identities = lock_identities(project['compiled_lock'])
        require(identities <= lock_identities((self.root / '.reference/teaql-rs/Cargo.lock').read_text()),
                'reference registry identities/checksums')
        metadata = project['metadata']
        repository = Path(source['owned_directory']) / 'repository'
        local = [p for p in metadata['packages'] if p['source'] is None]
        require(len(local) == len(r['local_packages']) and
                {p['name']: str(Path(p['manifest_path']).relative_to(repository)) for p in local}
                == r['local_packages'], 'original local package paths')
        require({(p['name'], p['version'], p['source']) for p in metadata['packages']
                 if p['source'] is not None} <= {p[:3] for p in identities}, 'metadata registry identities')
        require(len(source['builds']) == 2, 'original build count')
        artifacts = {}
        for build, profile in zip(source['builds'], ('debug', 'release')):
            require(build['profile'] == profile and build['compiled_lock'] == project['compiled_lock'],
                    'original build profile/lock')
            step = source['steps'][build['step']]
            require(self.same(step['exit_code'], 0) and '--offline' in step['command']
                    and '--locked' in step['command'], 'locked original compilation')
            require(('--release' in step['command']) is (profile == 'release'), 'compiler profile selection')
            observed = [x for x in self.lines(step['stdout']) if x.get('reason') == 'compiler-artifact'
                        and x.get('executable')]
            require(self.same(observed, build['artifacts']), 'raw original compiler artifacts')
            names = {a['target']['name'] for a in build['artifacts']}
            require(names == set(r['profile_targets'][profile]) and len(names) == len(build['artifacts']),
                    'complete original compiler targets')
            require(set(build['binary_hashes']) == names, 'original executable hashes')
            for artifact in build['artifacts']:
                name = artifact['target']['name']
                require(artifact['profile']['debug_assertions'] is (profile == 'debug'), 'original assertion profile')
                require(Path(artifact['target']['src_path']).resolve()
                        == (repository / r['drivers'][name]).resolve(), 'original driver path')
                require(Path(artifact['executable']) == Path(source['owned_directory']) / 'target' / profile / name,
                        'original executable path')
                require(re.fullmatch('[0-9a-f]{64}', build['binary_hashes'][name]) is not None,
                        'original executable identity')
                artifacts[profile, name] = artifact
        require(len(source['contracts']) == len(r['contracts']), 'source contract count')
        total = 0
        for actual, contract in zip(source['contracts'], r['contracts']):
            require(all(self.same(actual[k], contract[k]) for k in contract), 'contract identity/count/selection')
            rows = self.selected_cases(contract)
            inputs = [row['input'] for row in rows]
            expected = [row[contract['expected_key']]['64'] if contract['binary'] == 'decimal_pointer_oracle'
                        else row[contract['expected_key']] for row in rows]
            require(self.same(actual['inputs'], inputs) and self.same(actual['expected'], expected),
                    'complete source inputs/expectations')
            step = source['steps'][actual['step']]
            executable = artifacts[contract['profile'], contract['binary']]['executable']
            require(step['command'] == [executable] and self.same(step['exit_code'], 0), 'original execution')
            payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
            require(hashlib.sha256(payload.encode()).hexdigest() == step['stdin_sha256'], 'serialized source inputs')
            output = self.lines(step['stdout'])
            require(self.same(output, actual['actual']) and self.same(output, expected)
                    and actual['differences'] == [], 'complete original raw output')
            total += len(output)
        require(self.same(total, 13451) and self.same(source['observations'], total), 'source observation cardinality')
        return self

    def validate(self):
        self.validate_files()
        return self.validate_source(self.source)

    def native(self, directory):
        results = []
        for contract in self.registry['contracts']:
            if contract['profile'] != 'debug':
                continue
            binary = directory / self.registry['native_roles'][contract['binary']]
            before = digest(binary)
            special = {'decimal_pointer_oracle': ('--pointer-bits', '64'),
                       'audit_event_oracle': ('--pointer-bits', '64'),
                       'sqlite_decoding_oracle': ('--profile', 'checked')}
            if contract['binary'] in special:
                flag, wanted = special[contract['binary']]
                require(subprocess.check_output([str(binary), flag], text=True).strip() == wanted,
                        'current native width/arithmetic profile')
            inputs = [c['input'] for c in self.selected_cases(contract)]
            payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
            with tempfile.TemporaryDirectory(prefix='teaql_profile_native_506_') as name:
                process = subprocess.run([str(binary)], input=payload, cwd=name,
                    env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'), text=True, capture_output=True, check=True)
            require(digest(binary) == before, 'native executable changed')
            actual = self.lines(process.stdout)
            original = next(c['actual'] for c in self.source['contracts'] if all(c[k] == v for k, v in contract.items()))
            require(self.same(actual, original), 'current native/source outputs')
            results.append(dict(contract=contract, binary=str(binary), binary_sha256=before,
                count=len(actual), stdout=process.stdout, stderr=process.stderr))
        require(sum(r['count'] for r in results) == 7308 and len(results) == 7, 'complete native scope')
        return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--cpp-build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.verify_only == bool(args.cpp_build_dir):
        parser.error('Use --verify-only or --cpp-build-dir')
    if args.record and args.record.exists():
        parser.error('Preserve historical evidence')
    root = args.root.resolve()
    registry = json.loads((root / 'tests/parity/profile_public_cli_source_contracts_5_0_6.json').read_text())
    require(digest(Path(__file__)) == registry['verifier_sha256'], 'current source verifier')
    validator = Validator(root, registry).load().validate()
    results = [] if args.verify_only else validator.native(args.cpp_build_dir.resolve())
    record = dict(complete=True, whole_goal_complete=False, fresh_rust_execution=False,
                  reused_rust_execution=True, native_execution=not args.verify_only,
                  source_execution_sha256=registry['source_execution']['sha256'], results=results)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))
    print('Profile source provenance: six corpora / 13451 original profile outcomes')
    if results:
        print('Current native observers: 7308/7308 outcomes match; original execution reused')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
