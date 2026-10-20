"""Boundary inputs for the original query helpers and validation entry points."""
import itertools


def cases():
    result = []
    # Rust str::trim treats these whitespace boundaries differently from NUL,
    # zero-width space and the historical Mongolian vowel separator.
    namespaces = ['', ' ', '\t\r\n', '\u0085', '\u00a0', '\u1680', '\u2000',
                  '\u2007', '\u2028\u2029', '\u202f', '\u205f', '\u3000',
                  '\0', '\u180e', '\u200b', ' ns雪\0 ', 'n' * 4096]
    for kind, namespace, ttl, maximum in itertools.product(
            ('continuous', 'id_set'), namespaces,
            ('0', '1', '18446744073709551615'),
            ('0', '1', '18446744073709551615')):
        result.append(dict(kind='validate_query', type=kind, namespace=namespace,
                           ttl=ttl, maximum=maximum))
    for maximum in ('0', '1', '10000', '18446744073709551615'):
        result.append(dict(kind='validate_query', type='hard_limit', namespace='',
                           ttl='0', maximum=maximum))
    # Complete helper/model Debug and builder sequences preserve embedded NUL,
    # SQL-looking names and long multibyte strings without treating them as SQL.
    for kind, (column, pattern) in itertools.product(
            ('query_helpers', 'query_builders'),
            [('\0', 'before\0after'), ('"order";--', "'_%\\\n"),
             ('雪' * 128, '😀' * 256), ('\u200bname', '\u3000reason\u3000')]):
        result.append(dict(kind=kind, column=column, pattern=pattern,
                           value=dict(kind='text', value='value\0雪')))
    return result


def intent_cases():
    return [dict(entity=entity, comment=comment,
                 purposes=[purpose, 'finish\0' + entity, purpose])
            for entity, comment, purpose in itertools.product(
                ('Order', 'Entity\0雪', 'E' * 128),
                (None, 'before\0comment'),
                ('', ' ', '\u3000\t', '\0', 'Review rows', 'why\0雪',
                 '\u200breason', '😀' * 256))]
