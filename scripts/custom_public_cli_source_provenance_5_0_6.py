#!/usr/bin/env python3
"""Verify four custom source contracts and compare current native observers."""
import argparse
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


class QualificationError(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise QualificationError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def check_transition(text, original_sha256, transition):
    actual = hashlib.sha256(text.encode()).hexdigest()
    require(actual in [original_sha256, transition['candidate_sha256']],
            'declared comparator transition')
    if actual != original_sha256:
        restored = text
        for patch in reversed(transition['replacements']):
            require(restored.count(patch['new']) == 1, 'inverse comparator patch')
            restored = restored.replace(patch['new'], patch['old'])
        require(hashlib.sha256(restored.encode()).hexdigest() == original_sha256,
                'unmodified original comparator outside repair')


class Validator:
    def __init__(self, root, registry):
        self.root = root.resolve()
        self.registry = registry
        sys.path.insert(0, str(self.root / 'scripts'))
        sys.path.insert(0, str(self.root / 'tests/parity'))
        import archive_driver_provenance_5_0_6 as archive
        import compare_values
        from verify_constant_closure_5_0_6 import registry_identities
        self.archive = archive
        self.same = compare_values.same
        self.lines = compare_values.json_lines
        self.registry_identities = registry_identities
        self.modules = {}
        self.source = None
        self.binding = None

    def load(self):
        for name in ['source_execution', 'native_binding']:
            item = self.registry[name]
            require(digest(self.root / item['path']) == item['sha256'], name + ' artifact hash')
        self.source = read(self.root / self.registry['source_execution']['path'])
        self.binding = read(self.root / self.registry['native_binding']['path'])
        return self

    def validate_source(self, source):
        r = self.registry
        require(source['complete'] is True and source['native_execution'] is False,
                'original execution scope')
        require(source['runtime_commit'] == r['runtime_commit']
                and source['generator_commit'] == r['generator_commit'], 'source baseline')
        require(self.same(source['contracts'], r['contracts']), 'source contracts')
        require(self.same(source['source_files'], r['evidence_source_files']), 'source input identities')
        require(source['input_hashes_before'] == source['input_hashes_after'],
                'original execution input epoch')
        require(source['references_before'] == source['references_after'], 'original reference epoch')
        require(hashlib.sha256(source['generated_manifest'].encode()).hexdigest()
                == r['generated_manifest_sha256'], 'original generated manifest')
        require(hashlib.sha256(source['compiled_lock'].encode()).hexdigest()
                == r['compiled_lock_sha256'], 'original compiled lock')
        metadata = json.loads(source['project_metadata'])
        repository = Path(source['owned_directory']) / 'repository'
        local_entries = [p for p in metadata['packages'] if p['source'] is None]
        local = {p['name']: p for p in local_entries}
        require(len(local_entries) == len(local), 'duplicate local package identity')
        require(set(local) == set(r['local_packages']), 'local package set')
        for name, relative in r['local_packages'].items():
            require(Path(local[name]['manifest_path']) == repository / relative,
                    'local package path: ' + name)
        with tempfile.TemporaryDirectory(prefix='teaql_custom_source_lock_506_') as name:
            lock = Path(name) / 'Cargo.lock'
            lock.write_text(source['compiled_lock'])
            identities = self.registry_identities(lock)
            require(identities <= self.registry_identities(self.root / '.reference/teaql-rs/Cargo.lock'),
                    'original registry identities/checksums')
            metadata_registry = {(p['name'], p['version'], p['source'])
                                 for p in metadata['packages'] if p['source'] is not None}
            require(metadata_registry <= {item[:3] for item in identities},
                    'metadata registry package identities')
        require(set(source['compiler_artifacts']) == {c['target'] for c in r['contracts']},
                'original compiler targets')
        for contract in r['contracts']:
            artifact = source['compiler_artifacts'][contract['target']]
            require(artifact['profile']['debug_assertions'] is True
                    and artifact['profile']['overflow_checks'] is True,
                    'original Debug/overflow profile')
            require(Path(artifact['target']['src_path']).resolve()
                    == (repository / contract['driver']).resolve(),
                    'original observer path')
            require(artifact['target']['name'] == contract['target'], 'original observer target')
            result = next(v for v in source['results'] if v['name'] == contract['name'])
            require(self.same(result['count'], contract['count']), 'original observation count')
            step = source['steps'][result['source_step']]
            require(self.same(step['exit_code'], 0), 'original observer exit')
            require(len(self.lines(step['stdout'])) == contract['count'], 'original output cardinality')
            corpus = self.corpus(contract)
            payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n'
                              for row in corpus['cases'])
            require(hashlib.sha256(payload.encode()).hexdigest() == step['stdin_sha256'],
                    'original serialized input identity')

    def validate_binding(self, binding):
        require(binding['complete'] is True and binding['native_execution'] is True
                and binding['reused_rust_execution'] is True
                and binding['fresh_rust_execution'] is False, 'native binding scope')
        require(binding['source_execution_sha256'] == self.registry['source_execution']['sha256'],
                'native original-execution identity')
        require(self.same(binding['contracts'], self.registry['contracts']), 'native contracts')
        require(self.same(binding['observations'], 2093)
                and self.same(binding['contracts_matched'], 4), 'native binding counts')
        require(binding['input_hashes_before'] == binding['input_hashes_after'],
                'native execution input epoch')
        require(binding['references_before'] == binding['references_after'], 'native reference epoch')
        require(self.same(binding['controls_passed'], 31) and len(binding['controls']) == 31
                and all(c['passed'] is True for c in binding['controls']),
                'retained boundary controls')
        require(len(binding['results']) == 4, 'native result groups')
        require({v['name'] for v in binding['results']}
                == {c['name'] for c in self.registry['contracts']}, 'native result names')
        for contract in self.registry['contracts']:
            result = next(v for v in binding['results'] if v['name'] == contract['name'])
            step = binding['steps'][result['native_step']]
            require(self.same(step['exit_code'], 0), 'native observer exit')
            require(step['binary_sha256'] == result['native_binary_sha256'],
                    'native executable identity')
            require(self.same(self.lines(step['stdout']), result['native']),
                    'native raw stream identity')
            require(self.same(result['original'], self.source_rows(contract)),
                    'retained original stream identity')
            self.compare(contract, result['native'])

    def validate_files(self):
        r = self.registry
        require(self.same(json.loads((self.root / 'upstream.lock.json').read_text())['commit'],
                          r['runtime_commit']), 'current runtime lock')
        for name, wanted in [('teaql-rs', r['runtime_commit']),
                             ('teaql_code_gen', r['generator_commit'])]:
            path = self.root / '.reference' / name
            actual = subprocess.check_output(['git', '-C', str(path), 'rev-parse', 'HEAD'],
                                             text=True).strip()
            require(actual == wanted, 'current reference: ' + name)
            require(not subprocess.check_output(['git', '-C', str(path), 'status', '--porcelain',
                                                  '--untracked-files=no']).strip(),
                    'dirty reference: ' + name)
        ledger = json.loads((self.root / r['collector_metadata']).read_text())
        require(ledger['runtime_commit'] == r['runtime_commit']
                and ledger['generator_commit'] == r['generator_commit'], 'collector baseline metadata')
        for name, wanted in r['evidence_source_files'].items():
            if name == r['collector_metadata']:
                continue
            path = self.root / name
            if name in r['script_transitions']:
                check_transition(path.read_text(), wanted, r['script_transitions'][name])
            else:
                require(digest(path) == wanted, 'source/project/fixture changed: ' + name)
        for name, wanted in r['helper_files'].items():
            require(digest(self.root / name) == wanted, 'verifier helper: ' + name)
        raw = subprocess.check_output(['git', '-C', str(self.root / '.reference/teaql-rs'),
                                       'archive', r['runtime_commit']])
        require(hashlib.sha256(raw).hexdigest() == self.source['archive_sha256'],
                'original reference archive')
        files = {name: hashlib.sha256(value).hexdigest()
                 for name, value in self.archive.archive_files(raw).items()}
        require(len(files) == 610 and self.same(files, self.source['archive_files']),
                'complete original archive file map')
        for contract in r['contracts']:
            self.inputs(contract)

    def corpus(self, contract):
        path = self.root / contract['corpus']
        require(digest(path) == contract['corpus_sha256'], 'original corpus identity')
        corpus = read(path)
        require(type(corpus['cases']) is list and len(corpus['cases']) == contract['count'],
                'complete corpus cardinality')
        return corpus

    def inputs(self, contract):
        path = contract['script']
        if path not in self.modules:
            spec = importlib.util.spec_from_file_location('custom_source_' + Path(path).stem,
                                                         self.root / path)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            self.modules[path] = module
        values = list(getattr(self.modules[path], contract['function'])())
        require(type(values) is list and len(values) == contract['count'], 'input builder cardinality')
        require(self.same(values, [v['input'] for v in self.corpus(contract)['cases']]),
                'complete input builder matrix')
        return values

    def source_rows(self, contract):
        result = next(v for v in self.source['results'] if v['name'] == contract['name'])
        return self.lines(self.source['steps'][result['source_step']]['stdout'])

    def compare(self, contract, actual):
        original = self.source_rows(contract)
        corpus = self.corpus(contract)
        require(type(actual) is list and len(actual) == len(original) == contract['count'],
                'complete observer cardinality')
        if contract['projection'] == 'stable_common_and_weak_ownership':
            weak = dict(model='weak_arc', within_row=[True, False], after_row=[False, False])
            arena = dict(model='context_arena', within_row=[True, True], after_row=[True, True])
            for case, source, current in zip(corpus['cases'], original, actual):
                saved = case['source']
                require(self.same(saved.get('common'), source.get('common'))
                        and self.same(saved.get('ownership'), source.get('ownership')),
                        'original cache source contract')
                if case['input']['kind'] == 'ownership':
                    require(self.same(source.get('ownership'), weak)
                            and self.same(current.get('ownership'), arena),
                            'explicit cache ownership profiles')
                else:
                    require(self.same(source.get('common'), current.get('common')),
                            'complete cache common geometry')
                    require(bool(current.get('diagnostics')), 'initialized native diagnostics')
        else:
            expected = [case['expected'] for case in corpus['cases']]
            if contract['projection'] == 'exact_encoded':
                self.inputs(contract)
                encoded = self.modules[contract['script']].encoded
                require(encoded(expected) == encoded(original), 'original complete encoded JSON')
                require(encoded(original) == encoded(actual), 'native complete encoded JSON')
            else:
                require(self.same(expected, original), 'original complete JSON')
                require(self.same(original, actual), 'native complete JSON')

    def validate(self):
        self.validate_files()
        self.validate_source(self.source)
        self.validate_binding(self.binding)
        return self


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--registry', type=Path)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--cpp-transaction', type=Path)
    parser.add_argument('--cpp-cache', type=Path)
    parser.add_argument('--cpp-deferred', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    registry_path = args.registry or root / 'tests/parity/custom_public_cli_source_contracts_5_0_6.json'
    registry = json.loads(registry_path.read_text())
    require(digest(Path(__file__)) == registry['verifier_sha256'], 'current verifier source identity')
    binaries = dict(transaction=args.cpp_transaction, cache=args.cpp_cache, deferred=args.cpp_deferred)
    if args.verify_only:
        if any(binaries.values()):
            parser.error('--verify-only cannot execute native observers')
    elif not all(binaries.values()):
        parser.error('Supply all three native probes or use --verify-only')
    if args.record:
        require(not args.record.exists(), 'preserve previous evidence')
    validator = Validator(root, registry).load().validate()
    report = dict(complete=False, fresh_rust_execution=False, reused_rust_execution=True,
                  native_execution=not args.verify_only, whole_goal_complete=False, results=[])
    if not args.verify_only:
        for contract in registry['contracts']:
            binary = binaries[registry['native_roles'][contract['name']]].resolve()
            before = digest(binary)
            inputs = validator.inputs(contract)
            payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
            environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
            if contract['name'] != 'initialized_snapshot_cache':
                environment['TEAQL_LOG_ENDPOINT'] = 'off'
            with tempfile.TemporaryDirectory(prefix='teaql_custom_native_contract_506_') as name:
                process = subprocess.run([str(binary)], input=payload, cwd=name, env=environment,
                                         text=True, capture_output=True, check=True)
            require(digest(binary) == before, 'current native binary changed during execution')
            actual = validator.lines(process.stdout)
            validator.compare(contract, actual)
            report['results'].append(dict(name=contract['name'], count=len(actual),
                binary=str(binary), binary_sha256=before,
                stdin_sha256=hashlib.sha256(payload.encode()).hexdigest(),
                stdout=process.stdout, stderr=process.stderr))
    report['complete'] = True
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(report).encode(), mtime=0))
    print('Custom source provenance: 4 contracts / 2093 observations; original Rust execution reused')
    if not args.verify_only:
        print('Current native observers: 2093/2093 outcomes match under declared projections')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
