"""Record public Rust i18n calls, including Unicode and catalog error order."""
import argparse
import itertools
import json
from pathlib import Path
import re
import subprocess
from compare_values import observe

LANGUAGES = ['en', 'zh-CN', 'zh-TW', 'ja', 'ko', 'de', 'fr', 'es', 'pt', 'ar',
             'th', 'id', 'fil', 'uk', 'vi']
KEYS = ['checker.required', 'checker.invalidType', 'checker.min', 'checker.max',
        'checker.minLength', 'checker.maxLength', 'checker.contextRootMissing',
        'checker.contextRootMismatch', 'known', 'missing']


def catalog(default='en', locales=None):
    if locales is None:
        locales = {'en': {'messages': {'known': 'Default', 'checker.required': 'Custom {location}'},
                          'vocabulary': {'known': 'Default vocabulary'}}}
    return dict(schema='teaql.i18n/v1', defaultLocale=default, locales=locales)


def query(source=None, language='en'):
    result = dict(op='catalog', language=language, messages=KEYS, vocabulary=KEYS)
    if source is not None:
        result['source'] = source if isinstance(source, str) else json.dumps(source)
    return result


def inputs(root):
    cases = []
    aliases = ['', ' ', 'en---', 'en_US', 'zh', 'zh-hans', 'zh_Hant', 'zh-SG', 'ZH-HK',
               'zh-mo', 'cn', 'tw', 'tl', 'tl-PH', 'zh-Latn', 'zh-CN-extra', 'xx',
               '-en', 'fil_PH', 'en\x00', 'ＦＲ', 'en\u200b']
    for code in LANGUAGES:
        aliases.extend([code, code.upper(), code.replace('-', '_'), code + '-extra'])
    for code in aliases:
        cases.append(dict(op='language', code=code))
        cases.append(dict(op='language', code='\u2003' + code + '\u3000'))
    paths = [[], [''], ['', 'HTTPServer'], [0, 'name'], ['myList', 2, 'userName'],
             ['HTTPServer'], ['has[rawSuffixABC]'], ['a.bC'], ['a', '', 'B'],
             ['ßİΣ'], ['İΣ\U00010400'], ['\u2160x'], ['\u0345X'], ['a\u1f88Z'],
             ['foo_bar'], ['name\x00X'], ['😀Name'], ['订单', 7, 'shipToAddress']]
    for path, language in itertools.product(paths, LANGUAGES):
        cases.append(dict(op='path', path=path, language=language))
    # Inputs cover all mapped/predicate characters in the immutable facts.
    # Expected titles are observed through Rust's original translate_location.
    points = set()
    for name in ['language_case_tables.hpp', 'uppercase_tables.hpp']:
        source = (root / 'include/teaql/core/detail' / name).read_text()
        for first, last, step in re.findall(r'\{0x([0-9A-Fa-f]+),\s*0x([0-9A-Fa-f]+),\s*(\d+),', source):
            points.update(range(int(first, 16), int(last, 16) + 1, int(step)))
        for first, last in re.findall(r'\{0x([0-9A-Fa-f]+),\s*0x([0-9A-Fa-f]+)\}', source):
            points.update(range(int(first, 16), int(last, 16) + 1))
        for first in re.findall(r'\{0x([0-9A-Fa-f]+),\s*\{', source):
            points.add(int(first, 16))
    for point in sorted(points):
        cases.append(dict(op='path', language='en', path=[chr(point) + 'p', 'p' + chr(point)]))

    alternate = catalog('de', {
        'de': {'messages': {'known': 'German default', 'checker.required': 'German custom {location}'},
               'vocabulary': {'known': 'German vocabulary'}},
        'fr': {'messages': {'known': 'French override'}, 'vocabulary': {'known': 'French vocabulary'}},
    })
    for source, language in itertools.product([None, catalog(), alternate], LANGUAGES):
        cases.append(query(source, language))
    invalid = [None, [], True, 5, '', 'text', {}, {'schema': 'teaql.i18n/v1'}]
    for key in ['schema', 'defaultLocale', 'locales']:
        for value in [None, True, 3, [], 'unknown']:
            modified = catalog()
            modified[key] = value
            invalid.append(modified)
    modified = catalog(); modified['extra'] = 'no'; invalid.append(modified)
    invalid += [catalog('fr'), catalog(locales={}), catalog('EN'), catalog('en-US')]
    for code in ['EN', 'en-US', 'zh', 'zh-cn', 'tl', 'xx', ' en ', 'zh-latn']:
        invalid.append(catalog(locales={code: {'messages': {}, 'vocabulary': {}}}))
    for value in [None, 'text', [], 2, {}, {'messages': {}}, {'messages': {}, 'vocabulary': {}, 'extra': {}}]:
        invalid.append(catalog(locales={'en': value}))
    for namespace, value in itertools.product(['messages', 'vocabulary'], [None, [], 3, 'text']):
        modified = catalog(); modified['locales']['en'][namespace] = value; invalid.append(modified)
    for value in [None, True, 2, [], {}, '', ' ', '{unknown}', '{}', '{', '}', '{{input}}',
                  '{input{system}}', '{input}', '{location}{system}{input}{input_len}', '{INPUT}',
                  '{input_len}\n', '{\x00}']:
        for namespace in ['messages', 'vocabulary']:
            modified = catalog(); modified['locales']['en'][namespace]['aaa'] = value; invalid.append(modified)
    invalid += ['{', '[] trailing', '{"a":}', '{"schema":"\\uD800"}', '{"a":1e999}',
                '{"a":01}', '{"a":NaN}', '{"a":true,}', '{"a":"\x01"}',
                '{"schema":"wrong","defaultLocale":"xx","locales":false}']
    for source in invalid:
        # JSON string roots are encoded explicitly; raw malformed JSON is separate.
        cases.append(query(json.dumps(source) if not isinstance(source, str) or source in ['', 'text'] else source))
    # Duplicate JSON keys follow serde_json last-write semantics, not a custom parser policy.
    cases.append(query('{"schema":"wrong","schema":"teaql.i18n/v1","defaultLocale":"en",'
                       '"locales":{"en":{"messages":{"known":"first","known":"last"},"vocabulary":{}}}}'))
    values = [dict(kind='null'), dict(kind='typed_null'), dict(kind='bool', value=True),
              dict(kind='bool', value=False), dict(kind='text', value='订单😀é'),
              dict(kind='text', value=''), dict(kind='text', value='a\x00b'),
              dict(kind='i64', value='-9223372036854775808'),
              dict(kind='u64', value='18446744073709551615'),
              dict(kind='decimal', value='1.2300'), dict(kind='decimal', value='-0.000'),
              dict(kind='date', value='2024-02-29'), dict(kind='timestamp', value='-12345'),
              dict(kind='json', value={'z': [None, True, '😀'], 'a': -0.0}),
              dict(kind='object', value={}), dict(kind='list', value=[])]
    values += [dict(kind='f64', value=value) for value in ['-0', '1e-300', '1e300', 'NaN',
               'inf', '-inf', '0.1', '2.2250738585072014e-308', '5e-324', '1.7976931348623157e308']]
    template = catalog(locales={'en': {'messages': {key: '{location}|{system}|{input}|{input_len}' for key in KEYS}, 'vocabulary': {}}})
    for value, rule in itertools.product(values, range(8)):
        cases.append(dict(op='translate', language='en', source=json.dumps(template),
                          path=['myList', 2, 'HTTPName'], rule=rule, input=value, system=value))
    for language, rule in itertools.product(LANGUAGES, range(8)):
        cases.append(dict(op='translate', language=language, path=[], rule=rule,
                          input=dict(kind='text', value='订单😀é'), system=dict(kind='u64', value='10')))
    for path, system, value in itertools.product(['{system}', '{input}', '{input_len}', '{location}'],
                ['{input}', '{input_len}', '{system}'], ['{input_len}', '{system}', '{input}']):
        cases.append(dict(op='translate', language='en', source=json.dumps(template), path=[path], rule=0,
                          system=dict(kind='text', value=system), input=dict(kind='text', value=value)))
    checks = [dict(rule=0, path=['firstName']), dict(rule=2, path=['balance'],
              input=dict(kind='decimal', value='1.00'), system=dict(kind='i64', value='3')),
              dict(rule=0, path=[], message=''), dict(rule=0, path=[], message='Preserved')]
    for initial, code, module, alias in itertools.product(['en', 'fr'], ['ja', 'zh_Hant', 'xx', ' tl-PH '],
                                                         [None, '', 'de'], [False, True]):
        case = dict(op='context', language=initial, set=code, alias=alias, checks=checks,
                    source=json.dumps(alternate))
        if module is not None:
            case['module'] = module
        cases.append(case)
    return cases


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--rust', required=True); args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    cases = inputs(root); outputs = observe(args.rust, cases)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], note='Original public Rust language/catalog/translator/Context/module observations; Unicode inputs cover immutable case facts.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('i18n_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} language/i18n observations')


if __name__ == '__main__':
    main()
