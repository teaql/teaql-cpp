"""Build the original locked runtime with a field-only private-method bridge.

The reference checkout is never edited. A git archive supplies every source file;
only the bridge below is appended in the generated, ignored build directory.
"""
import argparse
import io
import json
import os
from pathlib import Path
import re
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[2]
BRIDGE = '''
impl<'a, E> EntityDataService<'a, E>
where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {
    #[doc(hidden)]
    pub async fn __teaql_cpp_prepare_id_set_page(&self, query: SelectQuery)
        -> Result<(SelectQuery, Option<u64>), DataServiceError<E::Error>> {
        self.prepare_id_set_page(query).await
    }
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-dir', type=Path, default=ROOT / 'build/rust-id-set-target')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify-upstream.py')], check=True)
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    generated = ROOT / 'build/rust-id-set-original'
    source = generated / 'original'
    archive = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql-rs'),
                                       'archive', lock['commit']])
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        for member in tar:
            destination = source / member.name
            if not destination.resolve().is_relative_to(source.resolve()):
                raise RuntimeError('Archive path escapes the generated source tree')
            if member.isdir():
                destination.mkdir(parents=True, exist_ok=True)
            elif member.isfile():
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(tar.extractfile(member).read())
                os.utime(destination, (member.mtime, member.mtime))
    resolved = source / 'teaql-runtime/src/data_service/resolved.rs'
    resolved.write_text(resolved.read_text() + BRIDGE)
    project = generated / 'probe'
    (project / 'src').mkdir(parents=True, exist_ok=True)
    manifest = (ROOT / 'tests/parity/rust-values/Cargo.toml').read_text()
    manifest = re.sub(r'path = "\.\./\.\./\.\./\.reference/teaql-rs/([^"]+)"',
                      lambda match: 'path = ' + json.dumps(str(source / match[1])), manifest)
    (project / 'Cargo.toml').write_text(manifest)
    (project / 'Cargo.lock').write_bytes((ROOT / 'tests/parity/rust-values/Cargo.lock').read_bytes())
    (project / 'src/main.rs').write_bytes((ROOT / 'tests/parity/id_set_page_oracle.rs').read_bytes())
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.target_dir.resolve()))
    subprocess.run(['cargo', 'build', '--locked', '--offline', '--manifest-path', str(project / 'Cargo.toml')],
                   env=env, check=True)
    (generated / 'baseline.json').write_text(json.dumps(dict(commit=lock['commit'], bridge=BRIDGE), indent=2) + '\n')
    print(args.target_dir.resolve() / 'debug/teaql-cpp-value-oracle')


if __name__ == '__main__':
    main()
