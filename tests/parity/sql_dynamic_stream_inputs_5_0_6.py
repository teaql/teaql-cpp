"""Dynamic executor entry inputs; real source decides all ordering and outcomes."""
from itertools import product


def cases():
    result = [dict(gate=gate, capture=capture, observe=observe, support=support,
                   fail_after=fail_after, size=size, poll=poll)
              for gate, capture, observe, support, fail_after, size, poll in product(
                  ('valid', 'owner', 'selection', 'unknown', 'compile'),
                  (False, True), (False, True), (False, True), (None, 0, 1), (0, 1, 7), (False, True))]
    assert len(result) == 720
    return result
