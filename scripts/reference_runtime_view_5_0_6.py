"""Development-only byte-checked view of private functions in the fixed runtime.

Keep all original source and test bodies. Append explicitly supplied observer
modules and omit only disabled external-exporter test dependencies. The caller
owns the disposable directory, command recording and actual test selection.
"""
import hashlib
import json
from pathlib import Path
import shutil

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ReferenceRuntimeView:
    def __init__(self, root, directory, observers, run):
        self.root = Path(root)
        self.directory = Path(directory)
        self.source = self.root / '.reference/teaql-rs/teaql-runtime'
        self.observers = {name: Path(path) for name, path in observers.items()}
        self.originals = {}
        self.prefixes = {}
        metadata = json.loads(run(['cargo', 'metadata', '--offline', '--locked', '--no-deps',
                                   '--format-version', '1', '--manifest-path',
                                   self.source / 'Cargo.toml']))
        package = next(row for row in metadata['packages']
                       if Path(row['manifest_path']) == self.source / 'Cargo.toml')
        if package['version'] != '5.0.6' or package['edition'] != '2024':
            raise RuntimeError('Unexpected private-runtime package contract')
        if package['features'] != {'opentelemetry': ['dep:opentelemetry']}:
            raise RuntimeError('Review runtime features before preparing a new source view')
        for name in ('src', 'tests', 'test-vectors'):
            # Cargo can reuse the root package fingerprint across disposable
            # views. Preserving old source mtimes can leave a newer test binary
            # from a different appended observer in place. Fresh copy mtimes
            # force compilation of this view while preserving every source byte.
            shutil.copytree(self.source / name, self.directory / name,
                            copy_function=shutil.copy)
            for path in (self.source / name).rglob('*'):
                if path.is_file():
                    relative = path.relative_to(self.source).as_posix()
                    self.originals[relative] = digest(path)
                    if digest(self.directory / relative) != self.originals[relative]:
                        raise RuntimeError(f'Copied original body differs: {relative}')
        for name, observer in self.observers.items():
            if name not in self.originals or not name.startswith('src/'):
                raise RuntimeError(f'Observer target is not an original runtime source: {name}')
            destination = self.directory / name
            self.prefixes[name] = destination.read_bytes()
            destination.write_bytes(self.prefixes[name] + b'\n' + observer.read_bytes())
        manifest = '[package]\nname="teaql-runtime"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
        self.omitted_dev_dependencies = []
        for kind, heading in ((None, 'dependencies'), ('dev', 'dev-dependencies')):
            manifest += '\n[' + heading + ']\n'
            for dependency in package['dependencies']:
                if dependency['kind'] != kind:
                    continue
                if kind == 'dev' and dependency['name'] != 'teaql-macros':
                    self.omitted_dev_dependencies.append(dependency['name'])
                    continue
                if dependency.get('target'):
                    raise RuntimeError('Review target-specific dependencies before preparing a view')
                fields = ['version=' + json.dumps(dependency['req'])]
                for key in ('path', 'features', 'optional'):
                    if dependency.get(key):
                        fields.append(key + '=' + json.dumps(dependency[key]))
                if not dependency.get('uses_default_features', True):
                    fields.append('default-features=false')
                manifest += dependency['name'] + '={' + ','.join(fields) + '}\n'
        if set(self.omitted_dev_dependencies) != {'opentelemetry_sdk', 'opentelemetry-otlp'}:
            raise RuntimeError('Unexpected omitted development dependency')
        manifest += '\n[features]\nopentelemetry=["dep:opentelemetry"]\n'
        (self.directory / 'Cargo.toml').write_text(manifest)
        reference_lock = self.root / '.reference/teaql-rs/Cargo.lock'
        shutil.copyfile(reference_lock, self.directory / 'Cargo.lock')
        run(['cargo', 'metadata', '--offline', '--format-version', '1', '--manifest-path',
             self.directory / 'Cargo.toml'])
        lock = self.directory / 'Cargo.lock'
        if not registry_identities(lock) <= registry_identities(reference_lock):
            raise RuntimeError('Private-runtime registry versions/checksums differ')
        reference_versions = versions(reference_lock)
        for name, resolved in versions(lock).items():
            if not resolved <= reference_versions.get(name, set()):
                raise RuntimeError(f'Private-runtime package version differs: {name}')
        self.verify()

    def verify(self):
        for name, expected in self.originals.items():
            path = self.directory / name
            if name in self.observers:
                if path.read_bytes() != self.prefixes[name] + b'\n' + self.observers[name].read_bytes():
                    raise RuntimeError(f'Original prefix or appended observer changed: {name}')
            elif digest(path) != expected:
                raise RuntimeError(f'Original source/test body changed: {name}')

    def provenance(self):
        self.verify()
        return {'source_files_sha256': self.originals,
                'manifest': (self.directory / 'Cargo.toml').read_text(),
                'lock': (self.directory / 'Cargo.lock').read_text(),
                'observers': {name: {'sha256': digest(path), 'text': path.read_text()}
                              for name, path in self.observers.items()},
                'omitted_dev_dependencies': self.omitted_dev_dependencies,
                'all_original_bodies_preserved': True}
