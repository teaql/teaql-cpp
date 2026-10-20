#!/usr/bin/env python3
"""Verify complete locked field-template review spans; executable acceptance is separate."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / '.reference/teaql_code_gen'
SOURCE = 'generator/src/main/resources/generator/rust-lib-core/request/index.stg'
RECORD = ROOT / 'docs/parity/predicate_projection_source_review_after_business_id_lookup_5_0_6.json'
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
CPP_SOURCE = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def requirement(name):
    if name.startswith('order_by_'):
        return 'PP11'
    if name.startswith('group_by_'):
        return 'PP03'
    if name.startswith(('count_', 'sum_', 'avg_', 'min_', 'max_', 'standard_deviation_',
                        'square_root_of_population_standard_deviation_',
                        'sample_variance_', 'sample_population_variance_')):
        return 'PP04'
    if name.startswith(('select_', 'project_', 'unselect_')):
        return 'PP02' if '_function' in name else 'PP01'
    if 'snakeBoolean' in name:
        return 'PP06'
    if name.startswith('create_') or name == '<fieldDescriptor.rustAttributePredicatePrefixExpr>':
        return 'PP07'
    if any(s in name for s in ('_contain', '_start', '_end', '_sound', '_not_contain',
                               '_not_start', '_not_end')):
        return 'PP09'
    if any(s in name for s in ('_before', '_after', '_between_range')):
        return 'PP10'
    if name.endswith(('_in', '_not_in')):
        return 'PP08'
    if name.endswith(('_is_unknown', '_is_known')):
        return 'PP05'
    if name.endswith(('_is', '_is_not', '_greater_than', '_greater_than_or_equal_to',
                      '_less_than', '_less_than_or_equal_to', '_between')):
        return 'PP08'
    raise ValueError('Unreviewed source responsibility: ' + name)


def closing_brace(text, opening):
    depth, quoted, escaped = 0, False, False
    for index in range(opening, len(text)):
        char = text[index]
        if quoted:
            if escaped:
                escaped = False
            elif char == '\\':
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char == '{':
            depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0:
                return index + 1
    raise ValueError('Unclosed source function')


def report():
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    source = subprocess.check_output(['git', '-C', str(REFERENCE), 'show',
                                     ledger['generator_commit'] + ':' + SOURCE]).decode()
    assert digest(source) == ledger['sources'][SOURCE]['sha256']
    start = source.index('fieldMethods(fieldDescriptor) ::= <<')
    end = source.index('scalarCandidateMethods(fieldDescriptor) ::= <<', start)
    template = source[start:end]
    functions = []
    for match in re.finditer(r'pub fn ([^\n(]+)\(', template):
        name = match[1].replace('\\<T\\>', '')
        opening = template.index('{', match.end())
        finish = closing_brace(template, opening)
        body = template[match.start():finish]
        guards = []
        for directive in re.finditer(r'<(if\([^>]*\)|else|endif)>', template[:match.start()]):
            instruction = directive[1]
            if instruction.startswith('if('):
                guards.append(instruction[3:-1])
            elif instruction == 'else':
                guards[-1] = 'NOT (' + guards[-1] + ')'
            else:
                guards.pop()
        functions.append(dict(name=name, requirement=requirement(name), guards=guards,
                              start_line=source[:start + match.start()].count('\n') + 1,
                              end_line=source[:start + finish].count('\n') + 1,
                              sha256=digest(body), body=body))
    assert len(functions) == 63, 'Review added/removed source methods explicitly'
    assert {item['requirement'] for item in functions} == {f'PP{i:02}' for i in range(1, 12)}
    cpp = (PRODUCER / CPP_SOURCE).read_text()
    helpers = []
    for name in ['sourceTemplateFlag', 'sourceNumericField', 'queryFieldCriteria', 'queryField',
                 'descriptorFields', 'models', 'modelDefinitions', 'camelAssist', 'assist']:
        match = re.search(r'^  private [^\n]+\b' + name + r'\(', cpp, re.M)
        assert match, name
        next_method = re.search(r'^  private ', cpp[match.end():], re.M)
        end = match.end() + next_method.start() if next_method else len(cpp)
        text = cpp[match.start():end]
        helpers.append(dict(name=name, start_line=cpp[:match.start()].count('\n') + 1,
                            end_line=cpp[:end].count('\n'), sha256=digest(text), body=text))
    runtime = ROOT / 'teaql_runtime/include/teaql/runtime/entity_query.hpp'
    runtime_text = runtime.read_text()
    begin = runtime_text.index('    explicit EntityQuery(UserContext&')
    finish = runtime_text.index('    EntityQuery(const EntityQuery&', begin)
    constructor = runtime_text[begin:finish]
    return dict(generator_commit=ledger['generator_commit'], runtime_commit=ledger['runtime_commit'],
                source=SOURCE, template_sha256=digest(template), functions=functions,
                cpp_source=CPP_SOURCE, cpp_helpers=helpers,
                runtime_source=str(runtime.relative_to(ROOT)),
                runtime_constructor_sha256=digest(constructor), runtime_constructor=constructor,
                notice='Complete source/guard review index, not semantic acceptance or proof from counts.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    current = report()
    if args.record:
        assert not RECORD.exists(), 'Preserve the previous review'
        RECORD.write_text(json.dumps(current, indent=2) + '\n')
    else:
        assert current == json.loads(RECORD.read_text()), 'Source body/guard review changed'
    print(f'Verified {len(current["functions"])} complete field-template methods and '
          f'{len(current["cpp_helpers"])} native generation helpers; semantics require execution evidence')


if __name__ == '__main__':
    main()
