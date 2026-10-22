#!/usr/bin/env python3
"""Run complete original enhancement methods through the actual runtime query boundary."""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--explicit-intent', action='store_true')
    parser.add_argument('--combined', action='store_true')
    args = parser.parse_args()
    if args.combined and args.explicit_intent:
        parser.error('Choose combined or explicit-intent, not both')
    if args.output.exists():
        raise RuntimeError('Preserve existing evidence; choose a new output path')
    observer = ROOT / 'tests/parity/relation_enhancement_complete_source_5_0_6.rs'
    record = dict(complete=False, steps=[], controller=Path(__file__).read_text(),
                  observer_sha256=hashlib.sha256(observer.read_bytes()).hexdigest())

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))

    def interrupted(signum, frame):
        raise KeyboardInterrupt(f'controller signal {signum}')

    signal.signal(signal.SIGTERM, interrupted)
    parent = ROOT / '.tools/oracles'
    parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='relation_enhancement_complete_', dir=parent) as directory:
        owned = Path(directory)
        env = dict(os.environ, CARGO_TARGET_DIR=str(owned/'target'), CARGO_INCREMENTAL='0',
                   CARGO_PROFILE_DEV_DEBUG='0', CARGO_PROFILE_TEST_DEBUG='0',
                   CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
        record['owned_output'] = directory
        record['free_bytes_before'] = os.statvfs(parent).f_bavail * os.statvfs(parent).f_frsize
        record['effective_rust_environment'] = {k: env[k] for k in env if k.startswith('CARGO_')}

        def run(command):
            command = list(map(str, command))
            child = subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, text=True, start_new_session=True)
            try:
                stdout, stderr = child.communicate()
            except BaseException:
                if child.poll() is None:
                    os.killpg(child.pid, signal.SIGTERM)
                    try:
                        child.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        os.killpg(child.pid, signal.SIGKILL)
                        child.wait()
                raise
            record['steps'].append(dict(command=command, exit_code=child.returncode,
                                        stdout=stdout, stderr=stderr))
            save()
            print('exit', child.returncode, ' '.join(command), flush=True)
            if child.returncode:
                raise subprocess.CalledProcessError(child.returncode, command, stdout, stderr)
            return stdout

        try:
            run(['python3', 'scripts/verify_upstream.py'])
            inputs = []
            record['input_corpora'] = {}
            for name in ['relation_enhancement_5_0_6_cases.json.gz', 'relation_enhancement_variants_5_0_6_cases.json.gz']:
                path = ROOT/'tests/parity'/name
                data = json.loads(gzip.decompress(path.read_bytes()))
                assert data['baseline'] == 'fe82a18982415f1746df9a2cf7c42f5e9abdc6d3'
                inputs.extend(row['input'] for row in data['cases'])
                record['input_corpora'][name] = hashlib.sha256(path.read_bytes()).hexdigest()
            assert len(inputs) == 377
            unchanged = copy.deepcopy(inputs)
            if args.explicit_intent or args.combined:
                def add_intent(value):
                    if isinstance(value, dict):
                        for key, child in value.items():
                            if key == 'query' and isinstance(child, dict):
                                child['comment'] = 'Read complete enhancement inputs'
                                child['purpose'] = 'Verify the real Context and executor boundary'
                            add_intent(child)
                    elif isinstance(value, list):
                        for child in value:
                            add_intent(child)
                add_intent(inputs)
            if args.combined:
                inputs = unchanged + inputs
            record['input_mode'] = 'combined' if args.combined else ('explicit_intent' if args.explicit_intent else 'unchanged')
            record['inputs'] = inputs
            input_path = owned/'inputs.json'
            input_path.write_text(json.dumps(inputs)+'\n')
            rendered = owned/'observer.rs'
            rendered.write_text(observer.read_text().replace('__ENHANCEMENT_INPUTS__', str(input_path)))
            bridge = owned/'fixture_bridge.rs'
            bridge.write_text(''.join('#[cfg(test)] #[path = '+json.dumps(str(ROOT/'tests/parity/rust_values/src'/name))+'] mod '+Path(name).stem+';\n'
                                     for name in ['fixture_value.rs','fixture_sql.rs']))
            record['fixture_sha256'] = {name: hashlib.sha256((ROOT/'tests/parity/rust_values/src'/name).read_bytes()).hexdigest()
                                        for name in ['fixture_value.rs','fixture_sql.rs']}
            view = ReferenceRuntimeView(ROOT, owned/'runtime', {'src/data_service/relation.rs': rendered,
                                                                 'src/lib.rs': bridge}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory/'Cargo.toml', '--jobs', '2', '--lib',
                          'cpp_complete_enhancement_observer::complete_enhancement_inputs', '--', '--nocapture', '--test-threads=1'])
            rows = [json.loads(line.split('COMPLETE_ENHANCEMENT_OBSERVATION ', 1)[1])
                    for line in output.splitlines() if 'COMPLETE_ENHANCEMENT_OBSERVATION ' in line]
            assert len(rows) == len(inputs) and len({row['case'] for row in rows}) == len(inputs)
            assert '1 passed; 0 failed' in output
            record.update(observations=rows, provenance=view.provenance(), complete=True)
        except BaseException as error:
            record['error'] = repr(error)
            raise
        finally:
            record['bytes_to_remove'] = int(subprocess.check_output(['du', '-sb', directory], text=True).split()[0])
            save()
    record['cleanup_complete'] = not Path(directory).exists()
    save()
    print(f'Observed {len(rows)} complete original enhancement outcomes; disposable build removed')


if __name__ == '__main__':
    main()
