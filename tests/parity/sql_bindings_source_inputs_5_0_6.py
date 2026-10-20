"""Authored complete binding/provenance branches; no policy implementation."""

def cases():
    policies = [None, 0, 1, 2, 3]
    output = [dict(kind='combine', left=a, right=b) for a in policies for b in policies]
    fields = ['plain', 'masked', 'password', 'token_alias', 'db_masked', 'absent', '', 'plain\0tail']
    for mode in range(4):
        for field in fields:
            for shape in range(14):
                output.append(dict(kind='expression', mode=mode, field=field, shape=shape))
    texts = ['', 'secret', '%_\\', 'a\0b', '\n雪', 'x' * 128, ' ', 'e\u0301']
    for op in range(12):
        for variant in range(6):
            for text in texts:
                output.append(dict(kind='like', op=op, variant=variant, text=text))
    sources = [[], [1], [3] * 9, [0, 1, 2, 3, 1, 2], [0] * 6, [2] * 6]
    for i, scope in enumerate(policies):
        for untrusted in [False, True]:
            for j, source_policies in enumerate(sources):
                for generated in [False, True]:
                    output.append(dict(kind='bindings', mode=(i+j) % 4, scope=scope,
                                       untrusted=untrusted, source_policies=source_policies,
                                       source_generated=generated, text=texts[(i+j) % len(texts)]))
    assert len(output) == 1169
    return output
