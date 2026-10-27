"""Verify versioned multi-file observer assembly without executing producer entry points."""
import ast
import copy
import hashlib
from pathlib import Path
import re
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def verify_assets(contract, receipt):
    assets = receipt['reproduction_sources']
    required = {contract[key] for key in ('producer', 'observer', 'input_generator')}
    required.update(contract['render_inputs'])
    assert set(contract['asset_versions']) == required
    for name, pin in contract['asset_versions'].items():
        snapshot = assets[name]
        assert pin['sha256'] == receipt['inputs']['runtime:' + name] == sha(snapshot), name
        assert pin['version'] in ('current', 'archived'), name
        assert (snapshot == (ROOT / name).read_text()) == (pin['version'] == 'current'), name
    for name, pin in contract['input_files'].items():
        assert pin == receipt['inputs']['runtime:' + name] == hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
    for name, pin in contract['compile_inputs'].items():
        assert pin == receipt['inputs']['runtime:' + name] == hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def generate_inputs(contract, receipt):
    """Execute both versions of the selected input function with checked data-file reads."""
    reads = set()

    class InputPath:
        def __init__(self, value):
            self.path = Path(value)

        def with_name(self, name):
            return InputPath(self.path.with_name(name))

        def read_bytes(self):
            name = str(self.path.relative_to(ROOT))
            expected = contract['input_files'][name]
            data = self.path.read_bytes()
            assert hashlib.sha256(data).hexdigest() == expected == receipt['inputs']['runtime:' + name]
            reads.add(name)
            return data

    name = contract['input_generator']
    results = []
    for label, source in [('archived', receipt['reproduction_sources'][name]), ('current', (ROOT / name).read_text())]:
        namespace = dict(__file__=str(ROOT / name), __name__=contract['name'] + '_' + label + '_inputs')
        exec(compile(source, name, 'exec'), namespace)
        namespace['Path'] = InputPath
        reads.clear()
        results.append(namespace[contract.get('input_function', 'inputs')]())
        assert reads == set(contract['input_files']), (contract['name'], label, reads)
    return results


def rendering_function(source, kind):
    """Select actual producer rendering statements, preserving their nested closures."""
    tree = ast.parse(source)
    if kind == 'function':
        node = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'render')
        assert [a.arg for a in node.args.args] == ['root', 'input_path']
    else:
        assert kind == 'inline'

        def assigns(statement, name):
            return isinstance(statement, ast.Assign) and any(
                isinstance(target, ast.Name) and target.id == name for target in statement.targets)

        blocks = [n.body for n in ast.walk(tree) if isinstance(n, ast.With)
                  and any(assigns(s, 'fixture') for s in n.body)
                  and any(assigns(s, 'rendered') for s in n.body)]
        assert len(blocks) == 1
        body = blocks[0]
        first = next(i for i, s in enumerate(body) if assigns(s, 'fixture'))
        last = next(i for i, s in enumerate(body) if assigns(s, 'rendered'))
        assert first < last
        node = ast.parse('def render(root, input_path, args):\n    pass\n').body[0]
        node.body = copy.deepcopy(body[first:last + 1]) + [ast.Return(ast.Name(id='rendered', ctx=ast.Load()))]
    selected = ast.Module(body=[copy.deepcopy(node)], type_ignores=[])
    ast.fix_missing_locations(selected)
    return selected


def render_observer(contract, receipt, observer):
    """Read only declared archived assembly inputs and reproduce the exact compiled bytes."""
    assets = receipt['reproduction_sources']
    reads = set()

    class SourcePath:
        def __init__(self, name=''):
            self.name = name

        def __truediv__(self, name):
            return SourcePath(str(Path(self.name) / name))

        def read_text(self):
            assert self.name in contract['render_inputs'], self.name
            reads.add(self.name)
            return assets[self.name]

        def __str__(self):
            return str(ROOT / self.name)

    template = assets[contract['observer']]
    assert template.count(contract['marker']) == 1
    assert 'include_str!("' + contract['marker'] + '")' in template
    paths = re.findall(r'include_str!\("([^"\n]+)"\)', observer['text'])
    assert len(paths) == 1
    embedded = Path(paths[0])
    assert embedded.is_absolute() and embedded.name == 'inputs.json'
    selected = rendering_function(assets[contract['producer']], contract['renderer'])
    namespace = {}
    exec(compile(selected, contract['producer'], 'exec'), namespace)
    arguments = (SourcePath(), embedded)
    if contract['renderer'] == 'inline':
        arguments += (SimpleNamespace(suite=contract['suite']),)
    rendered = namespace['render'](*arguments)
    assert reads == set(contract['render_inputs']), (reads, contract['render_inputs'])
    assert rendered == observer['text']
    retained = receipt.get('rendered_observer', receipt.get('observer_rendering', {}).get('rendered'))
    assert rendered == retained
    modules = re.findall(r'#\[\s*path\s*=\s*"([^"\n]+)"\s*\]', rendered)
    included = re.findall(r'include(?:_bytes)?!\("([^"\n]+)"\)', rendered)
    dependencies = {str(Path(p).relative_to(ROOT)) for p in modules + included}
    assert dependencies == set(contract['compile_inputs'])
    return str(embedded), dict(renderer=contract['renderer'], suite=contract.get('suite'),
                              producer_sha256=sha(assets[contract['producer']]),
                              selected_ast_sha256=sha(ast.dump(selected, include_attributes=False)),
                              assembly_inputs=sorted(reads), compile_inputs=sorted(dependencies),
                              asset_versions=contract['asset_versions'])
