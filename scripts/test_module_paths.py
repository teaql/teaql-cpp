#!/usr/bin/env python3
"""Negative controls for current structure/source evidence, never runtime parity."""
from copy import deepcopy
from pathlib import Path
import tempfile
import unittest

from verify_module_paths import safe_path, verify_packages, verify_source_map


class SourceOwnershipControls(unittest.TestCase):
    def setUp(self):
        self.lock = {'commit': 'active'}
        self.contract = {'baseline': 'active', 'packages': {'teaql_core': {'rust_crate': 'teaql-core'}}}
        self.inventory = {'commit': 'active', 'crates': ['teaql-core'],
                          'files': [{'path': 'teaql-core/src/lib.rs', 'sha256': 'original'}]}
        self.dispositions = {'commit': 'active', 'sources': [
            {'source': 'teaql-core/src/lib.rs', 'sha256': 'original', 'status': 'pending_review'}]}

    def verify(self):
        return verify_source_map(self.lock, self.contract, self.inventory, self.dispositions)

    def test_valid_pending_mapping_is_only_structural(self):
        self.assertEqual(self.verify(), 1)
        self.assertEqual(self.dispositions['sources'][0]['status'], 'pending_review')

    def test_old_baseline_is_rejected(self):
        self.contract['baseline'] = 'historical'
        with self.assertRaisesRegex(ValueError, 'runtime lock'):
            self.verify()

    def test_missing_source_is_rejected(self):
        self.dispositions['sources'].clear()
        with self.assertRaisesRegex(ValueError, 'coverage'):
            self.verify()

    def test_duplicate_source_is_rejected(self):
        self.inventory['files'].append(deepcopy(self.inventory['files'][0]))
        with self.assertRaisesRegex(ValueError, 'Duplicate'):
            self.verify()

    def test_stale_hash_is_rejected(self):
        self.dispositions['sources'][0]['sha256'] = 'rewritten'
        with self.assertRaisesRegex(ValueError, 'Stale'):
            self.verify()

    def test_missing_crate_responsibility_is_rejected(self):
        self.contract['packages'].clear()
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            self.verify()

    def test_duplicate_crate_responsibility_is_rejected(self):
        self.contract['packages']['teaql_runtime'] = {'rust_crate': 'teaql-core'}
        with self.assertRaisesRegex(ValueError, 'Multiple'):
            self.verify()


class PackageStructureControls(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='teaql_path_controls_')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.packages = {'teaql_core': {'rust_crate': 'teaql-core'}}
        self.write('teaql_core/CMakeLists.txt', 'add_library(teaql_core src/lib.cpp)\n')
        self.write('teaql_core/src/lib.cppm', 'export module teaql_core;\n')
        self.write('teaql_core/src/lib.cpp', 'int exported() { return 1; }\n')
        self.paths = ['teaql_core/src/lib.cpp', 'teaql_core/src/lib.cppm']

    def write(self, path, text):
        destination = self.root / path
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(text)

    def verify(self):
        return verify_packages(self.root, self.packages, self.paths)

    def test_valid_package_is_accepted(self):
        self.assertEqual(self.verify(), 2)

    def test_module_name_drift_is_rejected(self):
        self.write('teaql_core/src/lib.cppm', 'export module teaql_runtime;\n')
        with self.assertRaisesRegex(ValueError, 'Primary module'):
            self.verify()

    def test_target_name_drift_is_rejected(self):
        self.write('teaql_core/CMakeLists.txt', 'add_library(teaql_other src/lib.cpp)\n')
        with self.assertRaisesRegex(ValueError, 'target differs'):
            self.verify()

    def test_commented_target_is_not_accepted(self):
        self.write('teaql_core/CMakeLists.txt', '# add_library(teaql_core src/lib.cpp)\nadd_library(teaql_other src/lib.cpp)\n')
        with self.assertRaisesRegex(ValueError, 'target differs'):
            self.verify()

    def test_hyphen_path_is_rejected(self):
        self.paths.append('teaql_core/src/new-body.cpp')
        self.write(self.paths[-1], 'int added() { return 2; }\n')
        with self.assertRaisesRegex(ValueError, 'hyphen'):
            self.verify()

    def test_implementation_in_include_is_rejected(self):
        self.paths.append('teaql_core/include/body.cpp')
        self.write(self.paths[-1], 'int added() { return 2; }\n')
        with self.assertRaisesRegex(ValueError, 'outside package src'):
            self.verify()

    def test_parent_escape_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Invalid owned path'):
            safe_path(self.root, '../outside')

    def test_symlink_escape_is_rejected(self):
        (self.root / 'escape').symlink_to(self.root.parent, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'escapes'):
            safe_path(self.root, 'escape/outside')


if __name__ == '__main__':
    unittest.main()
