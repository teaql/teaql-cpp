#!/usr/bin/env python3
"""Compare C++ audit metadata with the frozen original model and Rust templates."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CORPUS = ROOT / 'tests/parity/generated_privacy_metadata_5_0_6_cases.json'
REFERENCE = ROOT / '.reference/teaql_code_gen'
CPP = ROOT.parent / 'teaql_code_gen_cpp'
SOURCE_FILES = [
    'model/src/main/java/com/skynet/codegenerator/meta/ObjectDescriptor.java',
    'model/src/main/java/com/skynet/codegenerator/naming/ObjectDescriptorNaming.java',
    'model/src/main/java/com/skynet/codegenerator/naming/FieldDescriptorNaming.java',
    'generator/src/main/resources/generator/rust-lib-core/entity/struct.rs.stg',
]
LAUNCHER = r'''
import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.*;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.nio.file.*;
import java.util.*;
public final class FrozenPrivacy {
    public static void main(String[] args) throws Exception {
        Path input = Path.of(args[0]), output = Path.of(args[1]);
        GenerateContext context = new GenerateContext("rust-lib-core", input.getFileName().toString());
        context.setEntranceFile(input.getFileName().toString());
        context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
        BaseDomainManager manager = new BaseDomainManager();
        Domain raw = manager.getDomain(manager.parse(context));
        List<Map<String, Object>> result = new ArrayList<>();
        for (ObjectDescriptor entity : new EntityDomain(raw).getObjectDescriptors()) {
            Map<String, Object> item = new LinkedHashMap<>();
            item.put("entity", RustObjectDescriptorNaming.rustStructName(entity));
            item.put("table", RustObjectDescriptorNaming.rustTableName(entity));
            item.put("service", entity.getDataService().getName());
            item.put("mask_model_keys", entity.getAuditMaskFields().stream()
                .map(FieldDescriptorNaming::canonicalPropertyName).toList());
            item.put("mask_storage_keys", entity.getAuditMaskFields().stream()
                .map(RustFieldDescriptorNaming::rustStorageFieldName).toList());
            item.put("audit_limit", entity.getAttributeValue("audit_value_max_len"));
            result.add(item);
        }
        Files.createDirectories(output);
        Files.writeString(output.resolve("metadata.json"), new ObjectMapper().writeValueAsString(result));
        for (GeneratedFile file : new DomainGenerationService().generateDomainFiles(raw, "rust-lib-core")) {
            Path target = output.resolve(file.getPath()).normalize();
            if (!target.startsWith(output)) { throw new IllegalArgumentException("Unsafe generated path"); }
            Files.createDirectories(target.getParent());
            Files.write(target, file.getData());
        }
    }
}
'''


def cases():
    pairs = [('', ''), ('name', ''), ('later_secret', ''),
             ('Secret Value,laterSecret', 'child-secret'),
             ('NAME,secretValue', 'LATER_SECRET'),
             ('platform', 'platform_id'), ('id,version', 'version'),
             ('name,name,secret_value', 'unknown'),
             ('missing', 'name'), ('__secret--value__', 'Child Secret'),
             ('child_secret', ''), ('platform,name', 'laterSecret,child_secret')]
    result = []
    for index, (parent, child) in enumerate(pairs):
        limit = '' if index % 3 == 0 else f' _audit_value_max_len="{17 + index}"'
        model = ('<root name="privacy_fixture" data_service="sqlite">'
                 '<platform name="string()"/>'
                 '<record platform="platform(contextRoot=true)" name="string()" '
                 f'secret_value="string()" _audit_mask_fields="{parent}"{limit}/>'
                 '<child _parent="record" later_secret="string()" child_secret="string()" '
                 f'_audit_mask_fields="{child}"/>'
                 '<grandchild _parent="child" _audit_mask_fields="LATER_SECRET" '
                 '_audit_value_max_len="53"/></root>')
        result.append((f'inheritance_{index}', model))
    return result


def classpath(workspace, destination):
    subprocess.run(['mvn', '-q', '-pl', 'generator', '-am', '-DskipTests', 'package'],
                   cwd=workspace, check=True)
    subprocess.run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath',
                    f'-Dmdep.outputFile={destination}'], cwd=workspace, check=True)
    return ':'.join(str(workspace / path) for path in
                    ['generator/target/classes', 'model/target/classes']) + ':' + destination.read_text().strip()


def cpp_metadata(models):
    result = []
    pattern = r'inline EntityDescriptor (\w+)::entity_descriptor\(Memory r\) \{(.*?)return d;'
    for entity, body in re.findall(pattern, models, re.S):
        def string_call(name):
            return json.loads(re.search(r'd\.' + name + r'\(("(?:\\.|[^"\\])*")\);', body).group(1))
        masks = re.search(r'd\.mask_fields\(\{(.*?)\}\);', body, re.S).group(1)
        limit = re.search(r'd\.audit_max_length\((\d+)\);', body)
        result.append(dict(entity=entity, table=string_call('table'), service=string_call('service'),
                           mask_storage_keys=json.loads('[' + masks + ']'),
                           audit_limit=limit.group(1) if limit else None))
    return sorted(result, key=lambda item: item['entity'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    records = []
    with tempfile.TemporaryDirectory(prefix='teaql_privacy_metadata_') as directory:
        work = Path(directory)
        original_cp = classpath(REFERENCE, work / 'original_classpath.txt')
        cpp_cp = classpath(CPP, work / 'cpp_classpath.txt')
        launcher = work / 'FrozenPrivacy.java'
        launcher.write_text(LAUNCHER)
        subprocess.run(['javac', '-cp', original_cp, str(launcher)], check=True)
        for name, xml in cases():
            case = work / name
            case.mkdir()
            model = case / 'model.xml'
            model.write_text(xml)
            original = case / 'original'
            generated = case / 'cpp'
            subprocess.run(['java', '-cp', str(work) + ':' + original_cp, 'FrozenPrivacy',
                            str(model), str(original)], check=True)
            source = sorted(json.loads((original / 'metadata.json').read_text()),
                            key=lambda item: item['entity'])
            for item in source:
                entity_files = [path for path in original.rglob('entity.rs')
                                if f'pub struct {item["entity"]} ' in path.read_text()]
                if len(entity_files) != 1:
                    raise SystemExit(f'{name}: original entity source missing or ambiguous')
                attribute = re.search(r'audit_mask_fields = "([^"]*)"', entity_files[0].read_text())
                emitted = [part.strip() for part in attribute.group(1).split(',') if part.strip()]
                if emitted != item['mask_model_keys']:
                    raise SystemExit(f'{name}: original template differs from original model masks')
                limit = re.search(r'audit_value_max_len = (\d+)', entity_files[0].read_text())
                if (limit.group(1) if limit else None) != item['audit_limit']:
                    raise SystemExit(f'{name}: original template differs from original audit limit')
            subprocess.run(['java', '-cp', cpp_cp, 'com.skynet.codegenerator.cpp.CppCoreGenerator',
                            str(model), str(generated), '--qualify-inheritance'], check=True)
            observed = cpp_metadata((generated / 'include/privacy_fixture/models.hpp').read_text())
            expected = [{key: value for key, value in item.items() if key != 'mask_model_keys'}
                        for item in source]
            if observed != expected:
                raise SystemExit(f'{name}: C++ metadata differs\nexpected={expected}\nactual={observed}')
            records.append(dict(case=name, model=xml, model_sha256=hashlib.sha256(xml.encode()).hexdigest(),
                                source=source))
    result = dict(schema=1, generator_commit=ledger['generator_commit'], runtime_commit=ledger['runtime_commit'],
                  scope='Original resolved mask membership, model/storage identity, table/service and explicit audit limit; not runtime privacy acceptance',
                  source_hashes={path: hashlib.sha256((REFERENCE / path).read_bytes()).hexdigest()
                                 for path in SOURCE_FILES}, cases=records)
    if args.check:
        if json.loads(CORPUS.read_text()) != result:
            raise SystemExit('Frozen privacy metadata corpus differs')
    else:
        CORPUS.write_text(json.dumps(result, indent=2) + '\n')
    if subprocess.check_output(['git', '-C', str(REFERENCE), 'status', '--porcelain']).strip():
        raise SystemExit('Frozen producer changed during observation')
    print(f'Compared {len(records)} original/C++ models and {sum(len(row["source"]) for row in records)} entity metadata records')


if __name__ == '__main__':
    main()
