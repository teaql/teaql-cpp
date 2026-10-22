#!/usr/bin/env python3
"""Run byte-preserved scoped privacy observations in an owned temporary view."""
import argparse
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
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Preserve existing evidence; choose a new output path')
    observer = ROOT / 'tests/parity/resolved_scope_privacy_source_5_0_6.rs'
    record = dict(complete=False, steps=[], controller=Path(__file__).read_text(),
                  observer_sha256=hashlib.sha256(observer.read_bytes()).hexdigest())

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))

    def interrupted(signum, frame):
        raise KeyboardInterrupt(f'controller signal {signum}')

    signal.signal(signal.SIGTERM, interrupted)
    parent = ROOT / '.tools/oracles'
    parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='resolved_scope_privacy_', dir=parent) as directory:
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
            view = ReferenceRuntimeView(ROOT, owned/'runtime', {'src/data_service/resolved.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory/'Cargo.toml', '--jobs', '2', '--lib',
                          'cpp_resolved_scope_privacy_observer::scope_privacy', '--', '--nocapture', '--test-threads=1'])
            rows = [json.loads(line.split('RESOLVED_SCOPE_PRIVACY_OBSERVATION ', 1)[1])
                    for line in output.splitlines() if 'RESOLVED_SCOPE_PRIVACY_OBSERVATION ' in line]
            assert len(rows) == 8 and len({row['case'] for row in rows}) == 8
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
    print('Observed 8 original scoped privacy outcomes; disposable build removed')


if __name__ == '__main__':
    main()
