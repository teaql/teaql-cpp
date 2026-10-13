#!/usr/bin/env python3
"""Render same-model original Rust entity fixtures with the clean frozen producer."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from generate_business_id_reference import LAUNCHER

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = {'types': 'tests/generator/types.xml', 'relations': 'tests/generator/relations.xml',
            'optional_types': 'tests/generator/optional_types.xml',
            'checkers': 'tests/generator/checkers.xml',
            'inheritance': 'tests/generator/inheritance.xml',
            'inheritance_graph': 'tests/generator/inheritance_graph.xml',
            'inheritance_variants': 'tests/generator/inheritance_variants.xml',
            'inheritance_business_ids': 'tests/generator/inheritance_business_ids.xml',
            'candidates': 'tests/generator/candidates.xml',
            'constants_scalar': 'tests/generator/constants_scalar.xml',
            'constants_root': 'tests/generator/constants_root.xml',
            'constants_numeric': 'tests/generator/constants_numeric.xml',
            'constants_catalog': 'tests/generator/constants_catalog.xml',
            'dynamic_json': 'tests/generator/dynamic_json.xml',
            'sample_fields': 'tests/generator/sample_fields.xml',
            'sample_fields_runtime': 'tests/generator/sample_fields_runtime.xml',
            'sample_fields_temporal': 'tests/generator/sample_fields_temporal.xml'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--fixtures', nargs='+', choices=FIXTURES, default=list(FIXTURES))
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    verification = ['python3', str(ROOT/'scripts/verify_generator_parity.py')]
    if not args.check:
        verification += ['--regenerating-fixtures', *args.fixtures]
    subprocess.run(verification, check=True)
    reference = ROOT/'.reference/teaql_code_gen'
    subprocess.run(['mvn', '-q', '-pl', 'generator', '-am', '-DskipTests', 'package'], cwd=reference, check=True)
    ledger = json.loads((ROOT/'docs/cpp_generator_parity_ledger.json').read_text())
    with tempfile.TemporaryDirectory(prefix='teaql_frozen_entity_') as directory:
        work = Path(directory)
        classpath = work/'classpath.txt'
        subprocess.run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath', f'-Dmdep.outputFile={classpath}'], cwd=reference, check=True)
        cp = ':'.join(str(reference/path) for path in ['generator/target/classes','model/target/classes']) + ':' + classpath.read_text().strip()
        launcher = work/'FrozenFixture.java'
        launcher.write_text(LAUNCHER)
        subprocess.run(['javac', '-cp', cp, str(launcher)], check=True)
        for fixture in args.fixtures:
            model = FIXTURES[fixture]
            output = work/fixture
            subprocess.run(['java','-cp',str(work)+':'+cp,'FrozenFixture',str(ROOT/model),str(output)],check=True)
            files = sorted(path for path in output.rglob('*') if path.is_file())
            provenance = dict(generator_commit=ledger['generator_commit'], runtime_commit=ledger['runtime_commit'], model=model,
                              model_sha256=hashlib.sha256((ROOT/model).read_bytes()).hexdigest(), hand_edited=False,
                              files={path.relative_to(output).as_posix():hashlib.sha256(path.read_bytes()).hexdigest() for path in files})
            (output/'PROVENANCE.json').write_text(json.dumps(provenance,indent=2)+'\n')
            target = ROOT/'tests/generator/parity'/f'{fixture}_rust'
            expected = {path.relative_to(output) for path in output.rglob('*') if path.is_file()}
            actual = {path.relative_to(target) for path in target.rglob('*') if path.is_file()}
            if args.check:
                if expected != actual or any((output/path).read_bytes() != (target/path).read_bytes() for path in expected):
                    raise SystemExit(f'Frozen generated {fixture} fixture differs')
            else:
                for path in actual-expected:
                    (target/path).unlink()
                for path in expected:
                    (target/path).parent.mkdir(parents=True,exist_ok=True)
                    shutil.copyfile(output/path,target/path)
            print(f'{"Verified" if args.check else "Generated"} {len(expected)} frozen {fixture} files')
    if subprocess.check_output(['git','-C',str(reference),'status','--porcelain']).strip():
        raise SystemExit('Frozen producer changed during reference generation')
    subprocess.run(['python3', str(ROOT/'scripts/verify_generator_parity.py')], check=True)


if __name__ == '__main__':
    main()
