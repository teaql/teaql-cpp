#!/usr/bin/env python3
"""Generate the complete original/native expression-root identity fixture offline."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'tests/generator/parity/expression_root_contracts'
SOURCE = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
SUPPORT = 'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files(directory):
    return {str(p.relative_to(directory)):digest(p) for p in directory.rglob('*')
            if p.is_file() and p.name != 'PROVENANCE.json'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record-initial', action='store_true')
    parser.add_argument('--refresh-cpp', action='store_true')
    parser.add_argument('--refresh-result-markers', action='store_true')
    parser.add_argument('--refresh-business-id-lookups', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    assert sum([args.record_initial,args.refresh_cpp,args.refresh_result_markers,args.refresh_business_id_lookups,args.check]) == 1
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    reference = ROOT / '.reference/teaql_code_gen'
    lock = json.loads((producer / 'cpp_generator.lock.json').read_text())
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    assert subprocess.check_output(['git','-C',str(reference),'rev-parse','HEAD'],text=True).strip() == lock['generator_baseline_commit']
    assert not subprocess.check_output(['git','-C',str(reference),'status','--porcelain'])
    assert subprocess.check_output(['git','-C',str(producer),'branch','--show-current'],text=True).strip() == 'cpp_support'
    with tempfile.TemporaryDirectory(prefix='teaql_expression_roots_') as temporary:
        work = Path(temporary)
        staged = []
        for language,repo,cache in [('rust',reference,'original'),('cpp',producer,'cpp')]:
            cp = ':'.join(str(repo/name) for name in ('generator/target/classes','model/target/classes'))
            cp += ':' + Path(f'/tmp/teaql_constant_metadata_{cache}_classpath.txt').read_text().strip()
            subprocess.run(['javac','-cp',cp,'-d',str(work),str(ROOT/'tests/generator/TypeConfigurationProbe.java')],check=True)
            output = work / language
            subprocess.run(['java','-cp',str(work)+':'+cp,'TypeConfigurationProbe',language,str(BASE/'model.xml'),str(output)],check=True,stdout=subprocess.DEVNULL)
            assert json.loads((output/'OBSERVATION.json').read_text())['generated']
            if language == 'cpp':
                subprocess.run([str(ROOT/'.tools/clang-format/bin/clang-format'),f'--style=file:{ROOT/".clang-format"}','--Werror','-i',*(str(p) for p in output.rglob('*.hpp'))],check=True)
            provenance=dict(generator_commit=lock['generator_baseline_commit'],runtime_commit=lock['runtime_rust_baseline'],model=str((BASE/'model.xml').relative_to(ROOT)),model_sha256=digest(BASE/'model.xml'),hand_edited=False,files=files(output))
            if language == 'cpp':
                provenance.update(formatter='18.1.8',generator_implementation_sha256={name:digest(producer/name) for name in (SOURCE,SUPPORT)})
                catalog_source='generator/src/main/java/com/skynet/codegenerator/workspace/JavaWorkspaceI18nSampleGenerator.java'
                provenance['generator_implementation_sha256'][catalog_source]=digest(producer/catalog_source)
                workflow_source='generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistWorkflows.java'
                provenance['generator_implementation_sha256'][workflow_source]=digest(producer/workflow_source)
                support_source='generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistSupport.java'
                provenance['generator_implementation_sha256'][support_source]=digest(producer/support_source)
            target = BASE / language
            if args.record_initial:
                assert not target.exists()
                staged.append((output,target,provenance))
            elif args.check or language == 'rust':
                assert files(target) == provenance['files'],language
                assert json.loads((target/'PROVENANCE.json').read_text()) == provenance,language
            else:
                if args.refresh_business_id_lookups:
                    # This retained fixture has no Business-ID fields; the full SDK
                    # proof independently checks its eligibility through real metadata.
                    from business_id_lookup_qualification import verify_file as verify_lookup
                    def verify_file(root, name, previous, generated):
                        return verify_lookup(root, name, previous, generated, [])
                elif args.refresh_result_markers:
                    from query_result_marker_qualification import verify_file
                else:
                    from expression_entry_qualification import verify_file
                previous=json.loads((target/'PROVENANCE.json').read_text())
                assert files(target)==previous['files']
                assert previous['files'].keys()==provenance['files'].keys()
                for name in provenance['files']:
                    verify_file(ROOT,name,(target/name).read_text(),(output/name).read_text())
                staged.append((output,target,provenance))
            print('Verified expression root',language,len(provenance['files']),'complete files')
        for output,target,provenance in staged:
            for source in output.rglob('*'):
                if source.is_file():
                    destination=target/source.relative_to(output)
                    destination.parent.mkdir(parents=True,exist_ok=True)
                    if not destination.exists() or destination.read_bytes()!=source.read_bytes():shutil.copyfile(source,destination)
            (target/'PROVENANCE.json').write_text(json.dumps(provenance,indent=2)+'\n')
        if args.record_initial:
            history=BASE/'cpp_before_entry_fix';assert not history.exists()
            shutil.copytree(BASE/'cpp',history)


if __name__ == '__main__':
    main()
