#!/usr/bin/env python3
"""Validate complete private-source programs and their registered native protocols."""
import argparse
import collections
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
from profile_public_cli_source_provenance_5_0_6 import lock_identities


class Validator:
    def __init__(self, root, registry):
        self.root, self.registry = root.resolve(), registry
        sys.path.insert(0, str(self.root / 'tests/parity'))
        from compare_values import json_lines, same
        self.lines, self.same = json_lines, same

    def load(self):
        self.sources = []
        for entry in self.registry['source_executions']:
            require(digest(self.root / entry['path']) == entry['sha256'], 'source execution artifact')
            self.sources.append(read(self.root / entry['path']))
        return self

    def validate_files(self):
        r = self.registry
        require(json.loads((self.root / 'upstream.lock.json').read_text())['commit'] == r['runtime_commit'],
                'current source baseline')
        for name, commit in [('teaql-rs', r['runtime_commit']), ('teaql_code_gen', r['generator_commit'])]:
            require(subprocess.check_output(['git', '-C', str(self.root / '.reference' / name),
                                            'rev-parse', 'HEAD'], text=True).strip() == commit,
                    'current reference identity')
        for path, wanted in r['support_files'].items():
            require(digest(self.root / path) == wanted, 'observer/helper identity: ' + path)
        import archive_driver_provenance_5_0_6 as archive
        data = subprocess.check_output(['git', '-C', str(self.root / '.reference/teaql-rs'),
                                       'archive', r['runtime_commit']])
        self.archive_sha256 = hashlib.sha256(data).hexdigest()
        self.archive_files = {name: hashlib.sha256(value).hexdigest()
                              for name, value in archive.archive_files(data).items()}

    def validate_source(self, source, entry):
        require(source['complete'] is True and source['whole_goal_complete'] is False, 'source scope')
        require(source['runtime_commit'] == self.registry['runtime_commit']
                and source['generator_commit'] == self.registry['generator_commit'], 'source baseline')
        require(source['input_hashes_before'] == source['input_hashes_after']
                and source['references_before'] == source['references_after'], 'source execution epoch')
        require(source['archive_sha256'] == self.archive_sha256
                and source['archive_files'] == self.archive_files, 'complete original archive')
        provenance = source['combined_provenance']
        require(provenance['all_original_bodies_preserved'] is True
                and set(provenance['omitted_dev_dependencies']) == {'opentelemetry_sdk', 'opentelemetry-otlp'},
                'source prefix and external dependency scope')
        require(provenance['source_files_sha256'] == entry['runtime_files'], 'complete runtime source/test files')
        for relative, wanted in provenance['source_files_sha256'].items():
            require(self.archive_files['teaql-runtime/' + relative] == wanted, 'original runtime prefix')
        require(hashlib.sha256(provenance['manifest'].encode()).hexdigest() == entry['manifest_sha256']
                and hashlib.sha256(provenance['lock'].encode()).hexdigest() == entry['lock_sha256'],
                'compiled project identity')
        require(lock_identities(provenance['lock']) <= lock_identities(
            (self.root / '.reference/teaql-rs/Cargo.lock').read_text()), 'registry identities/checksums')
        for relative, wanted in source['fixture_files'].items():
            require(digest(self.root / relative) == wanted, 'original fixture identity')
        artifact = source['compiler_artifact']
        require(artifact['profile']['debug_assertions'] is True
                and artifact['profile']['overflow_checks'] is True and artifact['profile']['test'] is True,
                'original test profile')
        owned = Path(source['owned_directory'])
        require(Path(artifact['target']['src_path']) == owned / 'runtime/src/lib.rs', 'original test root')
        require(Path(artifact['executable']).is_relative_to(owned / 'target/debug'), 'owned executable')
        build = next(s for s in source['steps'] if s['label'] == 'combined_build')
        require(self.same(build['exit_code'], 0)
                and all(flag in build['command'] for flag in ['--offline', '--locked', '--no-run', '--lib']),
                'complete locked compilation')
        require(any(self.same(row, artifact) for row in self.lines(build['stdout'])), 'raw compiler artifact')
        require(len(source['programs']) == entry['programs'], 'complete program set')
        payloads = collections.defaultdict(list)
        seen = collections.defaultdict(set)
        total = 0
        for index, program in enumerate(source['programs']):
            contract = next(c for c in self.registry['contracts']
                            if c['execution'] == entry['path'] and c['program'] == index)
            require(program['original'] == contract['original']
                    and program['original_sha256'] == contract['original_sha256'], 'program origin selection')
            require(digest(self.root / program['original']) == program['original_sha256'], 'original receipt identity')
            original = read(self.root / program['original'])
            old_steps = original.get('steps', original.get('commands'))
            old_command = next(s['command'] for s in old_steps
                               if 'test' in s['command'] and '--lib' in s['command'])
            selector = old_command[old_command.index('--lib') + 1]
            for observer in program['observers'].values():
                for patch in observer['replacements']:
                    if patch['old'] == 'cpp_graph_planner_scopes':
                        selector = selector.replace(patch['old'], patch['new'])
            require(program['selector'] == selector, 'original test selector')
            require(self.same(program['expected'], original['observations']), 'complete retained expectation')
            require(self.same(program['inputs'], original['inputs']) if 'inputs' in original else
                    program['inputs'] is None or contract['input_recipe'] == 'id_set', 'source input selection')
            if contract['input_recipe'] == 'id_set':
                path = self.root / 'tests/parity/id_set_page_5_0_6_cases.json.gz'
                require(digest(path) == original['historical_corpus_sha256'], 'ID-set input corpus')
                old = [c['input'] for c in read(path)['cases']]
                require(self.same(program['inputs'], old + [dict(v, purpose='Verify retained ID-set execution')
                                                            for v in old]), 'complete ID-set continuation')
            if program['inputs'] is not None:
                require(hashlib.sha256(json.dumps(program['inputs']).encode()).hexdigest()
                        == program['input_bytes_sha256'], 'serialized fixture inputs')
            require(set(program['observers']) == set(original['provenance']['observers']), 'program observer targets')
            for target, observer in program['observers'].items():
                previous = original['provenance']['observers'][target]
                require(observer['original_sha256'] == previous['sha256']
                        and observer['original_text'] == previous['text'], 'complete observer body')
                text = observer['adapted_text']
                for patch in reversed(observer['replacements']):
                    old, new = patch['old'], patch['new']
                    fixture = re.findall(r'#\[path\s*=\s*"([^"]+)"\]', observer['original_text'])
                    inputs = re.findall(r'include_str!\("([^"]+)"\)', observer['original_text'])
                    allowed = (old in fixture and Path(old).is_relative_to(self.root)
                               and str(Path(old).relative_to(self.root)) in source['fixture_files']
                               and new == str(owned / 'repository' / Path(old).relative_to(self.root)))
                    allowed |= old in inputs and new == program['input_path']
                    allowed |= (old == 'cpp_graph_planner_scopes'
                                and re.fullmatch(r'cpp_graph_planner_scopes_variant_[01]', new) is not None
                                and observer['original_text'].count(old) == 1)
                    require(allowed and new in text, 'declared observer relocation')
                    text = text.replace(new, old)
                require(text == observer['original_text']
                        and hashlib.sha256(text.encode()).hexdigest() == observer['original_sha256'],
                        'unmodified observer outside relocation')
                adapted = observer['adapted_text']
                if adapted not in seen[target]:
                    payloads[target].append(adapted)
                    seen[target].add(adapted)
            step = source['steps'][program['execution_step']]
            require(step['command'] == [artifact['executable'], program['selected_test'], '--exact',
                                        '--nocapture', '--test-threads=1']
                    and self.same(step['exit_code'], 0), 'independent selected test execution')
            require(program['selected_test'] == program['selector']
                    or program['selected_test'].endswith('::' + program['selector']), 'test selector identity')
            rows = [json.loads(line.partition(program['marker'])[2]) for line in step['stdout'].split('\n')
                    if program['marker'] in line]
            require('1 passed; 0 failed' in step['stdout'] and self.same(rows, program['actual'])
                    and self.same(rows, program['expected']) and program['match'] is True
                    and program['differences'] == [], 'complete raw source observations')
            require(len(rows) == contract['count'], 'program cardinality')
            total += len(rows)
        require(set(payloads) == set(provenance['observers']), 'complete appended target set')
        for target, blocks in payloads.items():
            text = '\n'.join(blocks)
            require(provenance['observers'][target]['text'] == text
                    and provenance['observers'][target]['sha256'] == hashlib.sha256(text.encode()).hexdigest(),
                    'exact combined observer recipe')
        require(self.same(total, entry['observations']) and self.same(source['observations'], total),
                'complete source cardinality')

    def validate(self):
        self.validate_files()
        require(len(self.sources) == 2 and len(self.registry['contracts']) == 29, 'complete source epochs/contracts')
        pairs = [(c['execution'], c['program']) for c in self.registry['contracts']]
        expected = {(e['path'], i) for e in self.registry['source_executions'] for i in range(e['programs'])}
        require(len(pairs) == len(set(pairs)) and set(pairs) == expected
                and len({c['test'] for c in self.registry['contracts']}) == 29, 'unique complete invocation bindings')
        for source, entry in zip(self.sources, self.registry['source_executions']):
            self.validate_source(source, entry)
        return self

    def validate_commands(self, registry):
        for contract in self.registry['contracts']:
            tests = [t for t in registry['tests'] if t['name'] == contract['test']]
            require(len(tests) == 1, 'registered invocation identity')
            command = tests[0]['command']
            require(len(command) >= 2 and command[1] == str(self.root / contract['comparator'])
                    and '--cpp' in command
                    and Path(command[command.index('--cpp') + 1]).name == contract['binary'],
                    'registered comparator/native role')
            selected = (Path(command[command.index('--original') + 1]) if '--original' in command
                        else self.root / contract['default_original'])
            require(selected == self.root / contract['original'], 'registered original dataset selection')
            require(all(arg == '--cpp-emulator=' for arg in command if arg.startswith('--cpp-emulator=')),
                    'native invocation emulator scope')
            expected = ['--attachment-observations'] if contract['comparator'].endswith(
                'compare_relation_loaded_state_5_0_6.py') else []
            require(contract['arguments'] == expected, 'native observer mode')
        return self

    def native(self, directory):
        results = []
        sources = {e['path']: s for e, s in zip(self.registry['source_executions'], self.sources)}
        for contract in self.registry['contracts']:
            program = sources[contract['execution']]['programs'][contract['program']]
            inputs = program['inputs']
            if contract['input_recipe'] == 'legacy_intent':
                inputs = [dict(v, bound_intent='verify legacy graph trace') for v in inputs]
            expected = program['actual']
            if contract['output_projection'] != 'whole':
                expected = [row[contract['output_projection']] for row in expected]
            payload = '' if inputs is None else ''.join(json.dumps(value) + '\n' for value in inputs)
            binary = directory / contract['binary']
            before = digest(binary)
            with tempfile.TemporaryDirectory(prefix='teaql_custom_program_native_506_') as name:
                result = subprocess.run([str(binary), *contract['arguments']], input=payload, cwd=name,
                    env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'), text=True, capture_output=True, check=True)
            require(digest(binary) == before, 'current native binary identity')
            actual = self.lines(result.stdout)
            require(self.same(actual, expected), 'complete current native/source observations')
            results.append(dict(test=contract['test'], binary=str(binary), arguments=contract['arguments'], binary_sha256=before,
                count=len(actual), stdin_sha256=hashlib.sha256(payload.encode()).hexdigest(),
                stdout=result.stdout, stderr=result.stderr))
        require(sum(r['count'] for r in results) == 8098, 'complete native program cardinality')
        return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--cpp-build-dir', type=Path)
    parser.add_argument('--ctest-build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.verify_only == bool(args.cpp_build_dir):
        parser.error('Use --verify-only or --cpp-build-dir')
    if args.record and args.record.exists():
        parser.error('Preserve historical evidence')
    root = args.root.resolve()
    registry = json.loads((root / 'tests/parity/custom_program_source_contracts_5_0_6.json').read_text())
    require(digest(Path(__file__)) == registry['verifier_sha256'], 'source verifier identity')
    validator = Validator(root, registry).load().validate()
    test_directory = args.ctest_build_dir or args.cpp_build_dir
    if test_directory:
        registered = json.loads(subprocess.check_output(['ctest', '--test-dir', str(test_directory),
                                                        '--show-only=json-v1'], text=True))
        validator.validate_commands(registered)
    results = [] if args.verify_only else validator.native(args.cpp_build_dir.resolve())
    if args.record:
        data = dict(complete=True, whole_goal_complete=False, fresh_rust_execution=False,
                    reused_rust_execution=True, native_execution=not args.verify_only, results=results)
        args.record.write_bytes(gzip.compress((json.dumps(data, indent=2) + '\n').encode(), mtime=0))
    print('Custom source programs: 29 complete datasets / 8098 original observations')
    if results:
        print('Current native programs: 8098/8098 outcomes match; original execution reused')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
