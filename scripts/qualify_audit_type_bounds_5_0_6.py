#!/usr/bin/env python3
"""Qualify source-faithful type promotion and bounds in an owned Java overlay."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / 'teaql_code_gen_cpp/generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java'


def candidate_text(text):
    replacements = [
        ('private record ParsedBounds(int count, int traits) {}',
         'private record ParsedBounds(int count, int traits, boolean trailing) {}'),
        ('''          first = 3;
        } else { first = metadataTypeKind(true); }''',
         '''          trivia();
          if (!take(')')) { return 0; }
          trivia();
          if (!take('+')) { return 3; }
          ParsedBounds rest = metadataBoundsAfter(false, false, true);
          return rest == null || rest.count() == 0 || rest.trailing() ? 0 : 4;
        } else { first = metadataTypeKind(true); }'''),
        ('''          return rest == null ? 0 : 4;
        }
        return 5;
      }
      boolean ranked''',
         '''          return rest == null || rest.count() == 0 || rest.trailing() ? 0 : 4;
        }
        return 5;
      }
      boolean ranked'''),
        ('''        return bounds == null || bounds.traits() == 0 ? 0 : bounds.count() == 1 ? 3 : 4;
      }
      if (!ranked)''',
         '''        return bounds == null || bounds.traits() == 0 ? 0 : 5;
      }
      if (!ranked)'''),
        ('''      int count = 1, traits = value == 1 || value == 4 ? 1 : 0;
      trivia();
      while (plus && take('+')) {
        if (!metadataBoundAhead(constants)) { break; }''',
         '''      int count = 1, traits = value == 1 || value == 4 ? 1 : 0;
      boolean trailing = false;
      trivia();
      while (plus && take('+')) {
        trailing = true;
        if (!metadataBoundAhead(constants)) { break; }'''),
        ('''        ++count;
        if (value == 1 || value == 4) { ++traits; }
        trivia();
      }
      return new ParsedBounds(count, traits);''',
         '''        ++count;
        trailing = false;
        if (value == 1 || value == 4) { ++traits; }
        trivia();
      }
      return new ParsedBounds(count, traits, trailing);'''),
        ('return new ParsedBounds(0, 0);', 'return new ParsedBounds(0, 0, false);'),
        ('private record ParsedPath(boolean qualified, boolean modStyle, boolean single) {}',
         'private record ParsedPath(boolean qualified, boolean modStyle, boolean single, boolean lastArguments) {}'),
        ('''      boolean modStyle = true, single = true;
      while (true) {
        trivia();''',
         '''      boolean modStyle = true, single = true, lastArguments = false;
      while (true) {
        lastArguments = false;
        trivia();'''),
        ('modStyle = false; trivia();', 'modStyle = false; lastArguments = true; trivia();'),
        ('new ParsedPath(qualified, modStyle, single);',
         'new ParsedPath(qualified, modStyle, single, lastArguments);'),
        ('''      if (path.modStyle() && text.startsWith("::", position)) { position += 2; trivia(); }
      if (take('(')) {''',
         '''      if (!path.lastArguments() && text.startsWith("::", position)) { position += 2; trivia(); }
      if (!path.lastArguments() && take('(')) {'''),
        ('if (!(mutSelf && self) && !metadataType())',
         'if ((!(mutSelf && self) || named) && !metadataType())'),
        ('''      if (take('(')) {
        int value = metadataBound(constants, captures);
        trivia();
        return value != 0 && take(')') ? value : 0;
      }
      boolean ranked = keyword("for");''',
         '''      if (take('(')) {
        int value = metadataTraitBound(constants);
        trivia();
        return value != 0 && take(')') ? value : 0;
      }
      return metadataTraitBound(constants);
    }

    int metadataTraitBound(boolean constants) {
      boolean ranked = keyword("for");'''),
    ]
    for before, after in replacements:
        count = 2 if before == 'new ParsedPath(qualified, modStyle, single);' else 1
        assert text.count(before) == count, before
        text = text.replace(before, after)
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--comparison', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--inputs', type=Path,
                        default=Path('tests/parity/audit_type_bounds_inputs_5_0_6.json'))
    args = parser.parse_args()
    assert not args.record.exists() and not args.comparison.exists()
    original = SOURCE.read_text()
    assert hashlib.sha256(original.encode()).hexdigest() == 'bb098a9968ca2d95a688276a8f0087c07b0d923ec8427fed960c6150e06c03e0'
    candidate = candidate_text(original)
    record = dict(complete=False, installed=False, scope=__doc__, original_source=original,
                  original_sha256=hashlib.sha256(original.encode()).hexdigest(),
                  candidate_source=candidate, candidate_sha256=hashlib.sha256(candidate.encode()).hexdigest(),
                  steps=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_audit_type_bounds_candidate_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            path = owned / 'CppRustLiteralMetadata.java'
            path.write_text(candidate)
            command = ['python3', 'scripts/verify_audit_grammar_branches_5_0_6.py',
                       '--extra-inputs', str(args.inputs),
                       '--candidate-parser', str(path), '--record', str(args.comparison)]
            result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
            record['steps'].append(dict(command=command, exit_code=result.returncode,
                                        stdout=result.stdout, stderr=result.stderr))
            result.check_returncode()
            comparison = json.loads(gzip.decompress(args.comparison.read_bytes()))
            assert comparison['complete'] is True
            record['comparison_sha256'] = hashlib.sha256(args.comparison.read_bytes()).hexdigest()
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        assert SOURCE.read_text() == original
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Qualified', len(comparison['cases']), 'source results; producer remains unchanged')


if __name__ == '__main__':
    main()
