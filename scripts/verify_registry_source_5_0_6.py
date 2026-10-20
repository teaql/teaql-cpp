#!/usr/bin/env python3
"""Recompile exact registry observers and compare complete native/imported results."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT/'tests/parity'
PROJECT = PARITY/'rust_registry_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from registry_source_inputs_5_0_6 import cases

MATRICES = (
    ('runtime_registry', 'runtime_registry_baseline_5_0_6_cases.json.gz', 321),
    ('registry_debug_scope', 'registry_debug_scope_baseline_5_0_6_cases.json.gz', 18),
    ('entity_decoder', 'entity_decoder_5_0_6_cases.json.gz', 198),
    ('json_graph_decoder', 'json_graph_decoder_5_0_6_cases.json.gz', 24576),
    ('runtime_graph', 'runtime_graph_5_0_6_cases.json.gz', 464),
    ('schema_runtime', 'schema_runtime_baseline_5_0_6_cases.json.gz', 384),
    ('registry_smart_list', 'registry_smart_list_source_5_0_6_cases.json.gz', 1296),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--modules', action='store_true')
    parser.add_argument('--bootstrap-corpus', action='store_true',
                        help='Create only the new SmartList corpus after original/native agreement')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, whole_registry_accepted=False, full_goal_accepted=False,
                  steps=[], matrices=[], profiles={}, scope='Registry original/frontend qualification; '
                  'whole-source and capability decisions remain separate.')
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(),
                                             compresslevel=1, mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                                    stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        payload = ''.join(json.dumps(value)+'\n' for value in inputs)
        output = run([binary], payload)
        lines = output.split('\n')
        if lines and not lines[-1]:
            lines.pop()
        values = [json.loads(line) for line in lines]
        assert len(values) == len(inputs), (binary, len(values), len(inputs))
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    with tempfile.TemporaryDirectory(prefix='teaql_registry_source_506_') as temporary:
        directory = Path(temporary)
        bridges = {'src/entity_runtime.rs': PARITY/'registry_source_entity_runtime_bridge_5_0_6.rs',
                   'src/registry.rs': PARITY/'registry_source_private_bridge_5_0_6.rs'}
        view = ReferenceRuntimeView(ROOT, directory/'runtime', bridges, run)
        project = directory/'observer'
        project.mkdir()

        def absolute_path(match):
            target = (PROJECT/match[1]).resolve()
            if target == ROOT/'.reference/teaql-rs/teaql-runtime':
                target = view.directory
            return 'path = '+json.dumps(str(target))

        manifest = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, (PROJECT/'Cargo.toml').read_text())
        (project/'Cargo.toml').write_text(manifest)
        shutil.copyfile(PROJECT/'Cargo.lock', project/'Cargo.lock')
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project/'Cargo.toml', '--bins'])
        registry = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                        view.directory/'Cargo.toml', '--lib', '--', '--list'])
        names = re.findall(r'^([^ ]+): test$', registry, re.M)
        assert len(names) == 273
        assert not any(name.startswith('registry::') for name in names)
        record['original_test_registry'] = names
        record['original_registry_embedded_tests'] = 0
        record['original_runtime_view'] = view.provenance()
        record['observer_manifest'] = manifest
        record['observer_lock_sha256'] = digest(project/'Cargo.lock')
        baseline = json.loads((ROOT/'upstream.lock.json').read_text())['commit']
        matrices = []
        for stem, filename, count in MATRICES:
            path = PARITY/filename
            if stem == 'registry_smart_list' and not path.exists():
                assert args.bootstrap_corpus, 'Use --bootstrap-corpus for the first exact source/native creation'
                driver = PARITY/'registry_smart_list_source_5_0_6.rs'
                corpus = dict(baseline=baseline, driver=str(driver.relative_to(ROOT)),
                              driver_sha256=digest(driver), cases=[{'input':v} for v in cases()])
            else:
                corpus = json.loads(gzip.decompress(path.read_bytes()))
            assert corpus['baseline'] == baseline and len(corpus['cases']) == count
            driver = corpus.get('driver', corpus.get('original_driver'))
            assert digest(ROOT/driver) == corpus.get('driver_sha256', corpus.get('original_driver_sha256'))
            values = [case['input'] for case in corpus['cases']]
            if stem == 'registry_smart_list':
                assert values == cases()
            binary = Path(env['CARGO_TARGET_DIR'])/'debug'/(stem+'_registry_source_506')
            original = observe(binary, values)
            expected = [case.get('expected') for case in corpus['cases']]
            if path.exists():
                assert same(original, expected), stem
            else:
                for case, output in zip(corpus['cases'], original):
                    case['expected'] = output
            matrices.append((stem, path, corpus, values, original))
            record['matrices'].append(dict(name=stem, cases=count, original_binary_sha256=digest(binary),
                                            frontends={}))
            save()
    units = ('runtime_registry_tests', 'entity_decoder_tests', 'runtime_graph_tests', 'schema_runtime_tests')
    names = [stem+'_rust_parity' for stem, _, _ in MATRICES]
    names[-1] = 'registry_smart_list_source_rust_parity'
    names += ['runtime_registry_contracts', 'entity_decoder_contracts', 'runtime_graph_contracts', 'schema_runtime_contracts']
    for profile in ['native']+(['modules_native'] if args.modules else []):
        modules = profile == 'modules_native'
        prefix = 'teaql_module_' if modules else 'teaql_'
        build = ROOT/('build_modules_native' if modules else 'build_native_current')
        binaries = build/'modules' if modules else build
        run(['cmake', '--preset', profile])
        targets = [prefix+('registry_smart_list_source' if s=='registry_smart_list' else s)+'_probe'
                   for s, _, _ in MATRICES]+[prefix+unit for unit in units]
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        for index, (stem, path, corpus, values, original) in enumerate(matrices):
            target = prefix+('registry_smart_list_source' if stem=='registry_smart_list' else stem)+'_probe'
            binary = binaries/target
            outputs = observe(binary, values)
            gaps = [dict(index=i, input=values[i], original=a, actual=b)
                    for i, (a,b) in enumerate(zip(original,outputs)) if not same(a,b)]
            record['matrices'][index]['frontends'][profile] = dict(binary_sha256=digest(binary),
                                                                  differences=gaps)
            save()
            assert not gaps, (profile, stem, len(gaps))
            if not path.exists():
                assert stem=='registry_smart_list' and args.bootstrap_corpus
                path.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
            record['matrices'][index]['corpus_sha256'] = digest(path)
        selected = [('module_' if modules else '')+name for name in names]
        junit = args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest', '--test-dir', build, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^('+'|'.join(selected)+')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert len(checks) == len(selected) and {row.attrib['name'] for row in checks} == set(selected)
        assert not any(row.find(tag) is not None for row in checks for tag in ('failure','error','skipped'))
        record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text())
        assert 'no work to do' in run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        save()
        print(profile, sum(count for _,_,count in MATRICES), 'exact observations;',len(checks),'checks', flush=True)
    record.update(complete=True, normalization='none', observations_per_frontend=sum(n for _,_,n in MATRICES),
                  controller_sha256=digest(Path(__file__).resolve()))
    save()


if __name__ == '__main__':
    main()
