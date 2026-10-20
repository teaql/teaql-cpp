#!/usr/bin/env python3
"""Reproduce unchanged Rust and narrowly repaired C++ aggregate alias SDKs."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from query_alias_qualification import metadata, verify_file

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'tests/generator/parity/query_alias_contracts'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files(directory):
    return {str(p.relative_to(directory)): digest(p) for p in directory.rglob('*')
            if p.is_file() and p.name != 'PROVENANCE.json'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--refresh-cpp-aliases', action='store_true')
    parser.add_argument('--refresh-cpp-identities', action='store_true')
    args = parser.parse_args()
    assert sum([args.check, args.refresh_cpp_aliases, args.refresh_cpp_identities]) == 1
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    reference=ROOT/'.reference/teaql_code_gen'
    producer=ROOT.parent/'teaql_code_gen_cpp'
    lock=json.loads((producer/'cpp_generator.lock.json').read_text())
    assert subprocess.check_output(['git','-C',str(reference),'rev-parse','HEAD'],text=True).strip()==lock['generator_baseline_commit']
    assert not subprocess.check_output(['git','-C',str(reference),'status','--porcelain'])
    paths={'rust':(reference,Path('/tmp/teaql_constant_metadata_original_classpath.txt')),
           'cpp':(producer,Path('/tmp/teaql_constant_metadata_cpp_classpath.txt'))}
    classpaths={lang:':'.join(str(repo/p) for p in ('generator/target/classes','model/target/classes'))+':'+cache.read_text().strip() for lang,(repo,cache) in paths.items()}
    with tempfile.TemporaryDirectory(prefix='teaql_query_alias_generation_') as temporary:
        work=Path(temporary)
        for language in ('rust','cpp'):
            cp=classpaths[language]
            subprocess.run(['javac','-cp',cp,'-d',str(work),str(ROOT/'tests/generator/TypeConfigurationProbe.java')],check=True)
            if language=='cpp':
                subprocess.run(['javac','-cp',cp,'-d',str(work),str(ROOT/'tests/generator/QueryAliasMetadataProbe.java')],check=True)
                aliases=metadata(BASE/'model.xml',work/'aliases.json',cp,work)
            output=work/language
            subprocess.run(['java','-cp',str(work)+':'+cp,'TypeConfigurationProbe',language,str(BASE/'model.xml'),str(output)],stdout=subprocess.DEVNULL,check=True)
            assert json.loads((output/'OBSERVATION.json').read_text())['generated']
            if language=='cpp':
                subprocess.run([str(ROOT/'.tools/clang-format/bin/clang-format'),f'--style=file:{ROOT/".clang-format"}','--Werror','-i',*(str(p) for p in output.rglob('*.hpp'))],check=True)
            provenance=dict(generator_commit=lock['generator_baseline_commit'],runtime_commit=lock['runtime_rust_baseline'],hand_edited=False,model=str((BASE/'model.xml').relative_to(ROOT)),model_sha256=digest(BASE/'model.xml'),files=files(output))
            if language=='cpp':
                path='generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
                provenance['formatter']='18.1.8'
                provenance['generator_implementation_sha256']={path:digest(producer/path), 'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template':digest(producer/'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template')}
                catalog_source='generator/src/main/java/com/skynet/codegenerator/workspace/JavaWorkspaceI18nSampleGenerator.java'
                provenance['generator_implementation_sha256'][catalog_source]=digest(producer/catalog_source)
                workflow_source='generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistWorkflows.java'
                provenance['generator_implementation_sha256'][workflow_source]=digest(producer/workflow_source)
                support_source='generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistSupport.java'
                provenance['generator_implementation_sha256'][support_source]=digest(producer/support_source)
            target=BASE/language
            if args.check or language=='rust':
                assert files(target)==provenance['files'], language
                assert json.loads((target/'PROVENANCE.json').read_text())==provenance, language
            else:
                previous=json.loads((target/'PROVENANCE.json').read_text())
                assert files(target)==previous['files']
                assert previous['files'].keys()==provenance['files'].keys()
                changes=0
                for name in provenance['files']:
                    changes+=verify_file(ROOT,name,(target/name).read_text(),(output/name).read_text(),aliases)
                history=BASE/('cpp_before_identity_fix' if args.refresh_cpp_identities else 'cpp_before_alias_fix');assert not history.exists()
                shutil.copytree(target,history)
                shutil.copytree(output,target,dirs_exist_ok=True)
                (target/'PROVENANCE.json').write_text(json.dumps(provenance,indent=2)+'\n')
                print('Verified restricted alias/literal changes:',changes)
            print('Verified',language,len(provenance['files']),'complete files')


if __name__=='__main__':main()
