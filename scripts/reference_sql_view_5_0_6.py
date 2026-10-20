"""Byte-preserved SQL source view with explicit test-only diagnostic observers."""
import hashlib
import json
from pathlib import Path
import shutil

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ReferenceSqlView:
    def __init__(self, root, directory, observer, run):
        self.root = Path(root)
        self.directory = Path(directory)
        self.source = self.root/'.reference/teaql-rs/teaql-sql'
        self.observer = Path(observer)
        metadata = json.loads(run(['cargo', 'metadata', '--offline', '--locked', '--no-deps',
                                   '--format-version', '1', '--manifest-path', self.source/'Cargo.toml']))
        package = next(p for p in metadata['packages'] if Path(p['manifest_path']) == self.source/'Cargo.toml')
        assert package['version'] == '5.0.6' and package['edition'] == '2024' and not package['features']
        self.originals = {}
        for name in ('src', 'tests', 'test-vectors'):
            if not (self.source/name).exists():
                continue
            shutil.copytree(self.source/name, self.directory/name, copy_function=shutil.copy)
            for path in (self.source/name).rglob('*'):
                if path.is_file():
                    relative = path.relative_to(self.source).as_posix()
                    self.originals[relative] = digest(path)
                    assert digest(self.directory/relative) == self.originals[relative]
        self.prefix = (self.directory/'src/lib.rs').read_bytes()
        (self.directory/'src/lib.rs').write_bytes(self.prefix+b'\n'+self.observer.read_bytes())
        manifest = '[package]\nname="teaql-sql"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
        for kind, section in ((None, 'dependencies'), ('dev', 'dev-dependencies')):
            manifest += '\n['+section+']\n'
            for dependency in package['dependencies']:
                if dependency['kind'] != kind:
                    continue
                assert not dependency.get('target')
                fields = ['version='+json.dumps(dependency['req'])]
                for key in ('path', 'features', 'optional'):
                    if dependency.get(key):
                        fields.append(key+'='+json.dumps(dependency[key]))
                if not dependency.get('uses_default_features', True):
                    fields.append('default-features=false')
                manifest += dependency['name']+'={'+','.join(fields)+'}\n'
            if kind is None:
                # Observer serialization only; already in the exact reference lock.
                manifest += 'serde_json="=1.0.150"\n'
        (self.directory/'Cargo.toml').write_text(manifest)
        reference_lock = self.root/'.reference/teaql-rs/Cargo.lock'
        shutil.copyfile(reference_lock, self.directory/'Cargo.lock')
        run(['cargo', 'metadata', '--offline', '--format-version', '1', '--manifest-path',
             self.directory/'Cargo.toml'])
        assert registry_identities(self.directory/'Cargo.lock') <= registry_identities(reference_lock)
        reference_versions = versions(reference_lock)
        for name, found in versions(self.directory/'Cargo.lock').items():
            assert found <= reference_versions.get(name, set()), (name, found)
        self.verify()

    def verify(self):
        for name, expected in self.originals.items():
            path = self.directory/name
            if name == 'src/lib.rs':
                assert path.read_bytes() == self.prefix+b'\n'+self.observer.read_bytes()
            else:
                assert digest(path) == expected, name

    def provenance(self):
        self.verify()
        return dict(source_files_sha256=self.originals, all_original_bodies_preserved=True,
                    observer_sha256=digest(self.observer), observer_text=self.observer.read_text(),
                    manifest=(self.directory/'Cargo.toml').read_text(),
                    lock=(self.directory/'Cargo.lock').read_text(),
                    observer_only_dependency='serde_json=1.0.150; no original dependency omitted')
