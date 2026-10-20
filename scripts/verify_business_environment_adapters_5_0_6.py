#!/usr/bin/env python3
"""Prove the narrow generated startup change against its complete before snapshot."""
import gzip
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
BEFORE = ROOT / 'docs/parity/business_environment_adapter_before_5_0_6.json.gz'


def function(text, name):
    match = re.search(r'inline\s+[^\n{]+\s+' + name + r'\s*\(', text)
    assert match, name
    opening = text.index('{', match.end())
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return match.start(), end, text[match.start():end]


def tokens(text):
    return re.findall(r'"(?:\\.|[^"\\])*"|[A-Za-z_]\w*|\d+|[^\s]', text)


def check_startup_calls(text):
    """Wrap expression statements; retain calls already inside a checked expression."""
    edits = []
    pattern = r'(?m)^([ \t]*)((?:[A-Za-z_]\w*::)?)serviceRuntimeFromConnection\('
    for match in re.finditer(pattern, text):
        depth = 1
        end = match.end()
        quoted = None
        escaped = False
        while depth:
            character = text[end]
            if quoted:
                if escaped:
                    escaped = False
                elif character == '\\':
                    escaped = True
                elif character == quoted:
                    quoted = None
            elif character in ('"', "'"):
                quoted = character
            else:
                depth += (character == '(') - (character == ')')
            end += 1
        terminator = re.match(r'\s*;', text[end:])
        if not terminator:
            continue
        call = text[match.start() + len(match[1]):end]
        replacement = match[1] + match[2] + 'require_value(' + call + ');'
        edits.append((match.start(), end + terminator.end(), replacement))
    for start, end, replacement in reversed(edits):
        text = text[:start] + replacement + text[end:]
    return text, len(edits)


def verify_file(name, old, new):
    if name.endswith('/runtime.hpp'):
        has_keys = 'assemble_generated_sqlite_runtime<DataServiceExecutor, true>' in old
        prefix = '''inline Result<Ref<DataServiceExecutor>, ServiceRuntimeError>
serviceRuntimeFromConnection(UserContext& user_context, DataServiceConnection connection) {
auto r = user_context.memory_resource(); using Output = Result<Ref<DataServiceExecutor>, ServiceRuntimeError>;
auto& module = moduleWithBehaviorsAndCheckers(user_context);
auto& schema_provider = user_context.make<LocalSchemaProvider>();
'''
        if has_keys:
            expected = prefix + '''auto prepared = assemble_generated_sqlite_runtime<DataServiceExecutor, true>(user_context, module, connection, schema_provider, install_generated_business_id_environment_key);
if (!prepared) { return Output::err(r, ServiceRuntimeError::from_preparation(r, prepared.error())); }
return Output::ok(r, prepared.value()); }'''
        else:
            expected = prefix + '''return Output::ok(r, assemble_generated_sqlite_runtime<DataServiceExecutor, false>(user_context, module, connection, schema_provider)); }'''
        assert tokens(function(new, 'serviceRuntimeFromConnection')[2]) == tokens(expected)
        expected_config = '''inline Result<Ref<DataServiceExecutor>, ServiceRuntimeError>
serviceRuntime(UserContext& user_context, const ServiceRuntimeConfig& config) {
auto r = user_context.memory_resource(); using Output = Result<Ref<DataServiceExecutor>, ServiceRuntimeError>;
if (config.memory_resource() != r) { throw std::invalid_argument("Generated runtime configuration requires an explicit destination copy"); }
auto connection = open_generated_sqlite_runtime_connection(user_context, config.databaseUrl());
if (!connection) { return Output::err(r, ServiceRuntimeError::from_preparation(r, connection.error())); }
return serviceRuntimeFromConnection(user_context, connection.value()); }'''
        assert tokens(function(new, 'serviceRuntime')[2]) == tokens(expected_config)
        restored = new
        for method in ['serviceRuntimeFromConnection', 'serviceRuntime']:
            start, end, _ = function(restored, method)
            restored = restored[:start] + function(old, method)[2] + restored[end:]
        for identifier, value in [('BUSINESS_ID_KEY_HEX_ENV', 'TEAQL_BUSINESS_ID_KEY_V1_HEX'),
                                  ('BUSINESS_ID_KEY_VERSION_ENV', 'TEAQL_BUSINESS_ID_KEY_VERSION')]:
            line = f'inline constexpr std::string_view {identifier} = "{value}";\n'
            if has_keys and identifier == 'BUSINESS_ID_KEY_VERSION_ENV':
                line += '\n'
            assert restored.count(line) == int(has_keys)
            restored = restored.replace(line, '', 1)
        assert restored == old, name
        return dict(runtime=True, has_keys=has_keys)
    if name.endswith('/support.hpp'):
        line = '#include <teaql/runtime/generated_business_id_environment.hpp>\n'
        assert new.count(line) == 1 and new.replace(line, '', 1) == old, name
        return dict(support=True)
    if name == 'ASSIST.md':
        previous = old.splitlines(keepends=True)
        current = new.splitlines(keepends=True)
        assert len(previous) == len(current), name
        count = 0
        for left, right in zip(previous, current):
            if left.startswith('`serviceRuntimeFromEnv'):
                assert 'check the Result before business execution' in right and 'manual typed-key' in right
                count += 1
            elif left.startswith('DataServiceBusinessIdAllocator names'):
                assert 'before executor registration' in right and 'BUSINESS_ID_KEY_HEX_ENV' in right
                count += 1
            else:
                assert left == right, (name, left, right)
        assert count in (1, 2)
        return dict(assist_paragraphs=count)
    assert new == old, name
    return {}


def main():
    before = json.loads(gzip.decompress(BEFORE.read_bytes()))
    count = 0
    numbered = 0
    for directory, files in before['sdks'].items():
        assert {str(p.relative_to(ROOT / directory)) for p in (ROOT / directory).rglob('*') if p.is_file()} == set(files)
        for name, record in files.items():
            if name in ('manifest.json', 'PROVENANCE.json'):
                continue
            outcome = verify_file(name, record['text'], (ROOT / directory / name).read_text())
            if outcome.get('runtime'):
                count += 1
                numbered += int(outcome['has_keys'])
    print(f'Preserved every unrelated body/data/workflow in {len(before["sdks"])} SDK destinations; '
          f'{count} thin startup adapters, {numbered} conditional key domains.')
    audit = json.loads(gzip.decompress(
        (ROOT / 'docs/parity/business_environment_adapter_caller_audit_v2_5_0_6.json.gz').read_bytes()))
    for filename, record in audit['callers'].items():
        expected, changed = check_startup_calls(record['before'])
        assert changed == record['checked_calls']
        assert tokens(expected) == tokens((ROOT / filename).read_text()), filename
    print(f'Verified {audit["checked_calls"]} checked startup Results in {audit["files"]} callers; '
          'all other caller tokens are unchanged.')


if __name__ == '__main__':
    main()
