"""Replay existing declarative input functions or exact producer expressions."""
import ast
import hashlib
import itertools
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def generate(contract):
    builder = contract['input_builder']
    path = ROOT / builder['path']
    source = path.read_text()
    assert hashlib.sha256(source.encode()).hexdigest() == builder['sha256']
    tree = ast.parse(source)
    if builder['kind'] == 'function':
        namespace = dict(__file__=str(path), __name__=contract['name'] + '_inputs')
        exec(compile(source, str(path), 'exec'), namespace)
        result = namespace[builder['name']]()
        selected = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == builder['name'])
    else:
        assert builder['kind'] == 'expression'
        values = [n.value for n in ast.walk(tree) if isinstance(n, ast.Assign)
                  and any(isinstance(t, ast.Name) and t.id == builder['name'] for t in n.targets)]
        assert len(values) == 1
        selected = values[0]
        result = eval(compile(ast.Expression(selected), str(path), 'eval'), dict(itertools=itertools))
    assert type(result) is list and len(result) == contract['count']
    return result, dict(path=builder['path'], source_sha256=builder['sha256'], kind=builder['kind'],
                        name=builder['name'], selected_ast_sha256=hashlib.sha256(
                            ast.dump(selected, include_attributes=False).encode()).hexdigest())
