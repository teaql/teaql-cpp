"""Admit complete retained generated matrices before running an observer."""


def require_cases(corpus, count, inputs=None):
    cases = corpus.get('cases')
    if type(cases) is not list or not cases:
        raise RuntimeError('Comparison corpus cases must be a nonempty list')
    if len(cases) != count:
        raise RuntimeError(f'Comparison corpus requires exactly {count} retained cases')
    if any(type(row) is not dict or set(row) != {'input', 'expected'} for row in cases):
        raise RuntimeError('Expression cases require exactly input and expected fields')
    if inputs is not None:
        from compare_values import same
        if not same([row['input'] for row in cases], list(inputs)):
            raise RuntimeError('Comparison corpus inputs differ from the complete source recipe')
    return cases
