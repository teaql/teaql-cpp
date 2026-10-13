#!/usr/bin/env python3
"""Reproduce the Business-ID Rust fixture using the clean frozen producer."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = '''import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import java.nio.file.*;
public final class FrozenFixture {
  public static void main(String[] args) throws Exception {
    Path input = Path.of(args[0]), output = Path.of(args[1]);
    GenerateContext context = new GenerateContext("rust-lib-core", input.getFileName().toString());
    context.setEntranceFile(input.getFileName().toString());
    context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
    BaseDomainManager manager = new BaseDomainManager();
    Domain domain = manager.getDomain(manager.parse(context));
    for (GeneratedFile file : new DomainGenerationService().generateDomainFiles(domain, "rust-lib-core")) {
      Path target = output.resolve(file.getPath()).normalize();
      if (!target.startsWith(output)) { throw new IllegalArgumentException("Unsafe generated path"); }
      Files.createDirectories(target.getParent());
      Files.write(target, file.getData());
    }
  }
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py'),
                    '--skip-business-id-fixture'], check=True)
    reference = ROOT / '.reference/teaql_code_gen'
    subprocess.run(['mvn', '-q', '-pl', 'generator', '-am', '-DskipTests', 'package'],
                   cwd=reference, check=True)
    with tempfile.TemporaryDirectory(prefix='teaql_frozen_business_id_') as directory:
        work = Path(directory)
        classpath = work / 'classpath.txt'
        subprocess.run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath',
                        f'-Dmdep.outputFile={classpath}'], cwd=reference, check=True)
        cp = ':'.join(str(reference / path) for path in
                      ['generator/target/classes', 'model/target/classes'])
        cp += ':' + classpath.read_text().strip()
        source = work / 'FrozenFixture.java'
        source.write_text(LAUNCHER)
        subprocess.run(['javac', '-cp', cp, str(source)], check=True)
        output = work / 'output'
        model = 'tests/generator/business_ids.xml'
        subprocess.run(['java', '-cp', str(work) + ':' + cp, 'FrozenFixture',
                        str(ROOT / model), str(output)], check=True)
        ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
        files = sorted(path for path in output.rglob('*') if path.is_file())
        manifest = dict(generator_commit=ledger['generator_commit'],
                        runtime_commit=ledger['runtime_commit'], model=model,
                        model_sha256=hashlib.sha256((ROOT / model).read_bytes()).hexdigest(),
                        hand_edited=False,
                        files={path.relative_to(output).as_posix(): hashlib.sha256(
                            path.read_bytes()).hexdigest() for path in files})
        (output / 'PROVENANCE.json').write_text(json.dumps(manifest, indent=2) + '\n')
        target = ROOT / 'tests/generator/parity/business_ids_rust'
        expected = {path.relative_to(output) for path in output.rglob('*') if path.is_file()}
        actual = {path.relative_to(target) for path in target.rglob('*') if path.is_file()}
        if args.check:
            if expected != actual or any((output / path).read_bytes() != (target / path).read_bytes()
                                         for path in expected):
                raise SystemExit('Frozen Business-ID generated Rust fixture differs')
        else:
            for path in actual - expected:
                (target / path).unlink()
            for path in expected:
                (target / path).parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(output / path, target / path)
        print(f'{"Verified" if args.check else "Generated"} {len(expected)} frozen Rust fixture files')
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)


if __name__ == '__main__':
    main()
