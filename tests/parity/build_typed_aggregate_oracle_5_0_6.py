"""Build the complete locked runtime with one append-only private-method bridge."""

import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / 'tests/parity/rust_typed_aggregate_5_0_6'
BUILD = ROOT / 'build_rust_typed_aggregate_5_0_6'
SOURCE = BUILD / 'original'
TARGET = Path('/tmp/teaql_cpp_typed_aggregate_rust_506')
BRIDGE_PATH = 'teaql-runtime/src/data_service/resolved.rs'
BRIDGE = """
impl<'a, E> EntityDataService<'a, E>
where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync,
{
    #[doc(hidden)]
    pub async fn __cpp_typed_aggregate<T: Entity>(&self, query: &SelectQuery,
        aggregates: &[RelationAggregate]) -> Result<SmartList<T>, DataServiceError<E::Error>> {
        self.fetch_entities_with_relation_aggregates_internal(query, aggregates).await
    }
}
"""


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    if lock['commit'] != 'fe82a18982415f1746df9a2cf7c42f5e9abdc6d3':
        raise RuntimeError('This access bridge is reviewed for the locked 5.0.6 source only')
    archive = subprocess.check_output([
        'git', '-C', str(ROOT / '.reference/teaql-rs'), 'archive', lock['commit']])
    files = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            destination = SOURCE / member.name
            if not destination.resolve().is_relative_to(SOURCE.resolve()):
                raise RuntimeError('Archive path escapes the derived source tree')
            if member.isfile():
                original = stream.extractfile(member).read()
                appended = BRIDGE.encode() if member.name == BRIDGE_PATH else b''
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(original + appended)
                actual = destination.read_bytes()
                if actual != original + appended:
                    raise RuntimeError(f'Original source changed: {member.name}')
                if not appended:
                    os.utime(destination, (member.mtime, member.mtime))
                files[member.name] = dict(original_sha256=digest(original),
                                          compiled_sha256=digest(actual),
                                          appended_bytes=len(appended))
    if BRIDGE_PATH not in files:
        raise RuntimeError('Bridge target absent from original archive')
    # Compare registry identities/checksums as well as versions, before compiling.
    import sys
    sys.path.insert(0, str(ROOT / 'scripts'))
    from verify_state_baselines_5_0_6 import versions
    original_versions = versions(SOURCE / 'Cargo.lock')
    for name, resolved in versions(PROJECT / 'Cargo.lock').items():
        if name != 'teaql_typed_aggregate_506' and not resolved <= original_versions.get(name, set()):
            raise RuntimeError(f'Observer dependency version changed: {name} {resolved}')
    from build_private_save_oracles_5_0_6 import registry_packages
    if registry_packages(PROJECT / 'Cargo.lock') - registry_packages(SOURCE / 'Cargo.lock'):
        raise RuntimeError('Observer registry identity/checksum differs from original')
    provenance = dict(schema=1, baseline=lock['commit'], files=files,
                      appended_bridges={BRIDGE_PATH: BRIDGE}, build_status='prepared',
                      driver_sha256=digest((PROJECT / 'src/main.rs').read_bytes()),
                      manifest_sha256=digest((PROJECT / 'Cargo.toml').read_bytes()),
                      lock_sha256=digest((PROJECT / 'Cargo.lock').read_bytes()),
                      reference_lock_sha256=digest((SOURCE / 'Cargo.lock').read_bytes()),
                      builder_sha256=digest(Path(__file__).read_bytes()))
    report = BUILD / 'provenance.json'
    report.write_text(json.dumps(provenance, indent=2) + '\n')
    if args.prepare_only:
        return
    env = dict(os.environ, CARGO_TARGET_DIR=str(TARGET),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
               TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(PROJECT / 'Cargo.toml')], env=env, check=True)
    provenance.update(build_status='completed', original_registry_versions_preserved=True,
                      binary_sha256=digest((TARGET / 'debug/teaql_typed_aggregate_506').read_bytes()))
    report.write_text(json.dumps(provenance, indent=2) + '\n')
    print(f'Complete runtime built; {len(files)} original archive files verified; one access bridge')


if __name__ == '__main__':
    main()
