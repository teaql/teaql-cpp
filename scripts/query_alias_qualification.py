"""Allow only original default result-key repairs and their exact Assist lines."""
import json
import re
import subprocess
from pathlib import Path

PREFIXES = {'aggregateCountField': ('', '_count'), 'aggregateSum': ('sum_', ''),
            'aggregateAvg': ('avg_', ''), 'aggregateMin': ('min_', ''),
            'aggregateMax': ('max_', ''), 'aggregateStddev': ('stdDev_', ''),
            'aggregateStddevPop': ('stdDevPop_', ''), 'aggregateVarSamp': ('varSamp_', ''),
            'aggregateVarPop': ('varPop_', '')}


def metadata(model, output, classpath, work):
    subprocess.run(['java', '-cp', str(work) + ':' + classpath,
                    'QueryAliasMetadataProbe', str(model), str(output)],
                   capture_output=True, text=True, check=True)
    return json.loads(output.read_text())['aliases']


def rewrite_queries(text, aliases):
    count = [0]
    pattern = (r'(query_\.(aggregateCountField|aggregateSum|aggregateAvg|aggregateMin|aggregateMax|'
               r'aggregateStddev|aggregateStddevPop|aggregateVarSamp|aggregateVarPop)\s*\(\s*'
               r'"(?:[^"\\]|\\.)*"\s*,\s*)"((?:[^"\\]|\\.)*)"')
    def replace(match):
        function = match[2]
        field = json.loads(re.search(r'\(\s*("(?:[^"\\]|\\.)*")', match[1])[1])
        prefix, suffix = PREFIXES[function]
        candidates = []
        for row in aliases:
            if row['canonical'] != field or (row['object'] and function != 'aggregateCountField'):
                continue
            cpp_member = row['cpp_count_member'] if function == 'aggregateCountField' else row['cpp_member']
            source_member = row['source_count_member'] if function == 'aggregateCountField' else row['source_member']
            old, new = prefix + cpp_member + suffix, prefix + source_member + suffix
            if json.loads('"' + match[3] + '"') in {old, new}:
                candidates.append(new)
        assert candidates and len(set(candidates)) == 1, (field, function, match[3], candidates)
        result = match[1] + json.dumps(candidates[0], ensure_ascii=False)
        count[0] += result != match[0]
        return result
    return re.sub(pattern, replace, text), count[0]


def rewrite_guide(text, aliases):
    methods = {}
    for row in aliases:
        if row['object']:
            continue
        for operation, method in row['methods'].items():
            value = operation + '_' + row['source_member']
            assert method not in methods or methods[method] == value, (method, value)
            methods[method] = value
    count = [0]
    pattern = r'(- Scalar aggregate: `(\w+)\(\)` uses alias `)([^`]*)(`;)'
    def replace(match):
        method = match[2]
        assert method in methods, method
        assert match[3] in {method, json.dumps(methods[method], ensure_ascii=False)}, match[0]
        result = match[1] + json.dumps(methods[method], ensure_ascii=False) + match[4]
        count[0] += result != match[0]
        return result
    text = re.sub(pattern, replace, text)
    differences = []
    for row in aliases:
        for canonical in row['projection']:
            identity = row['identities'][canonical]
            if canonical != identity:
                pair = json.dumps(canonical, ensure_ascii=False) + ' -> ' + json.dumps(identity, ensure_ascii=False)
                if pair not in differences:
                    differences.append(pair)
    if differences and 'Source metadata boundary:' not in text:
        addition = ('Source metadata boundary: query projections retain storage names while descriptors retain the original Rust member identities: `'
                    + ', '.join(differences) + '`. Queries of these fields can preserve the original unknown-field SQL failure. The adapter does not silently rename query fields to make that source configuration succeed.\n')
        assert text.count('## Business IDs\n') == 1
        text = text.replace('## Business IDs\n', addition + '## Business IDs\n')
        count[0] += 1
    return text, count[0]


def rewrite_models(text, aliases):
    entities = {}
    for row in aliases:
        entities.setdefault(row['cpp_type'], row)
    count = 0
    for name, row in entities.items():
        differences = {key: value for key, value in row['identities'].items() if key != value}
        if not differences:
            continue
        begin = re.search(r'inline EntityDescriptor\s+' + re.escape(name) + r'::entity_descriptor\(Memory r\)', text)
        assert begin, name
        following = re.search(r'^inline ', text[begin.end():], re.M)
        end = begin.end() + following.start() if following else len(text)
        body = text[begin.start():end]
        for canonical, identity in differences.items():
            pattern = r'(d\.property\(\s*)' + re.escape(json.dumps(canonical, ensure_ascii=False)) + r'(\s*,)'
            body, changed = re.subn(pattern, lambda match: match[1] + json.dumps(identity, ensure_ascii=False) + match[2], body)
            count += changed
        text = text[:begin.start()] + body + text[end:]
        begin = re.search(r'struct\s+' + re.escape(name) + r'\s*:', text)
        assert begin, name
        identity = text.index('QUERY_IDENTITY_FIELDS', begin.end())
        end = text.index('};', identity) + 2
        projection = 'static constexpr std::array<std::string_view, ' + str(len(row['projection'])) + '> QUERY_PROJECTION_FIELDS{' + ', '.join(json.dumps(field, ensure_ascii=False) for field in row['projection']) + '};'
        if not re.search(r'QUERY_PROJECTION_FIELDS', text[end:text.index('private:', end)]):
            text = text[:end] + '\n' + projection + text[end:]
            count += 1
    return text, count


def verify_file(root, name, previous, generated, aliases):
    if name == 'ASSIST.md':
        expected, count = rewrite_guide(previous, aliases)
    elif name.endswith('/queries.hpp') or name.endswith('/models.hpp'):
        expected, count = (rewrite_models(previous, aliases) if name.endswith('/models.hpp')
                           else rewrite_queries(previous, aliases))
        expected = subprocess.run([str(root / '.tools/clang-format/bin/clang-format'),
                                   f'--style=file:{root / ".clang-format"}', '--Werror',
                                   '--assume-filename=' + name], input=expected,
                                  text=True, capture_output=True, check=True).stdout
    else:
        assert previous == generated, 'Alias repair changed an unrelated generated file: ' + name
        return 0
    if expected != generated:
        import difflib
        difference = ''.join(difflib.unified_diff(expected.splitlines(keepends=True), generated.splitlines(keepends=True), fromfile='expected', tofile='generated'))
        raise AssertionError('Alias repair changed unrelated code/instructions: ' + name + '\n' + difference)
    return count
