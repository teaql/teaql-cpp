#!/usr/bin/env python3
"""Prepare intact locked runtime sources with one append-only access bridge."""
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[2]
DESTINATION = ROOT / 'build_rust_protected_behavior_5_0_6/original'
BRIDGE = '''
impl<'a, E> EntityDataService<'a, E>
where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {
    #[doc(hidden)]
    pub async fn __cpp_protected_delete(&self, command: &teaql_core::DeleteCommand,
        reason: Option<&str>) -> Result<u64, DataServiceError<E::Error>> {
        match reason {
            Some(reason) => self.with_mutation_intent(teaql_core::MutationIntent::new(reason)
                .map_err(crate::RuntimeError::from).map_err(DataServiceError::Runtime)?)
                .delete_internal(command).await,
            None => self.delete_internal(command).await,
        }
    }
    #[doc(hidden)]
    pub async fn __cpp_protected_recover(&self, command: &teaql_core::RecoverCommand,
        reason: Option<&str>) -> Result<u64, DataServiceError<E::Error>> {
        match reason {
            Some(reason) => self.with_mutation_intent(teaql_core::MutationIntent::new(reason)
                .map_err(crate::RuntimeError::from).map_err(DataServiceError::Runtime)?)
                .recover_internal(command).await,
            None => self.recover_internal(command).await,
        }
    }
}
'''


def main():
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    archive = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql-rs'),
                                       'archive', lock['commit']])
    records = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if not member.isfile():
                continue
            path = DESTINATION / member.name
            if not path.resolve().is_relative_to(DESTINATION.resolve()):
                raise RuntimeError('Archive escapes the derived source directory')
            original = stream.extractfile(member).read()
            suffix = BRIDGE.encode() if member.name == 'teaql-runtime/src/data_service/resolved.rs' else b''
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(original + suffix)
            actual = path.read_bytes()
            if actual != original + suffix:
                raise RuntimeError(f'Original bytes changed: {member.name}')
            records[member.name] = dict(original_sha256=hashlib.sha256(original).hexdigest(),
                                       compiled_sha256=hashlib.sha256(actual).hexdigest(),
                                       appended_bytes=len(suffix))
    (DESTINATION.parent / 'provenance.json').write_text(json.dumps(dict(
        runtime_commit=lock['commit'], bridge=BRIDGE, files=records,
        notice='Complete original archive remains a byte-identical prefix; the bridge only calls original private methods.'), indent=2) + '\n')
    print(f'Prepared {len(records)} intact source files with one append-only access bridge')


if __name__ == '__main__':
    main()
