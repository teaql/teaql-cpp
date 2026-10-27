"""Bound existing SDK changes to JSON sample delegation into the shared runtime."""
import re
import subprocess


def verify_file(root, relative, previous, generated, models):
    if not relative.startswith('include/') or not relative.endswith('/sample_data.hpp'):
        raise RuntimeError('Sample operand migration changed an unrelated SDK file: ' + relative)
    literal = r'"(?:\\.|[^"\\])*"'
    pattern = (r'GeneratedSampleValues::assign_operand\(\s*entity,\s*(' + literal + r'),\s*'
               r'entity\.([A-Za-z_][A-Za-z_0-9]*),\s*Value\(r,\s*'
               r'(GeneratedSampleValues::text\(.*?\bindex\))\)\);')
    count = 0

    def restore(match):
        nonlocal count
        field, member, expression = match.groups()
        assignments = re.findall(r'root\.set\(entity_key\(\),\s*' + re.escape(field)
                                 + r',\s*generated_detail::encode\(memory_resource\(\),\s*'
                                 r'([A-Za-z_][A-Za-z_0-9]*)\)\);', models)
        if set(assignments) != {member}:
            raise RuntimeError('Sample delegate changed its native storage identity: ' + field)
        if not re.search(r'\b(?:std::optional<)?Json>?\s+' + re.escape(member) + r';', models):
            raise RuntimeError('Existing sample migration is limited to JSON native storage')
        count += 1
        return ('{ auto original = ' + expression + '; '
                'entity.root.set(entity.entity_key(), ' + field + ', Value(r, original)); '
                'auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots(); '
                'entity.load_state.mark_loaded(' + field + ', &pool).value(); }')

    restored = re.sub(pattern, restore, generated, flags=re.S)
    if not count:
        raise RuntimeError('No reviewed JSON sample delegate found: ' + relative)
    formatted = subprocess.run([str(root / '.tools/clang-format/bin/clang-format'),
                                '--style=file:' + str(root / '.clang-format'), '--Werror',
                                '--assume-filename=' + relative], input=restored,
                               text=True, capture_output=True, check=True).stdout
    if formatted != previous:
        raise RuntimeError('Sample migration changed an existing non-delegate body: ' + relative)
    return count
