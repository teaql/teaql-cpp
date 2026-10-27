"""Canonical inputs from the retained configuration execution collectors.

Keep product order and JSON value types identical to the original recipes in
scripts/verify_{constant_metadata,constant_topology,type_configuration}_execution.py
and scripts/verify_scalar_candidate_execution.py. These describe observer inputs,
not generated application APIs or original-source execution evidence.
"""

import itertools


def bootstrap_inputs():
    return [dict(state=state, module=module, rounds=rounds, logging=logging)
            for state, module, rounds, logging in itertools.product(
                ('fresh', 'existing', 'drift', 'missing', 'deleted', 'extra'),
                ('plain', 'checker', 'behavior', 'combined'), (1, 2), (False, True))]


def scalar_candidate_inputs(count):
    if type(count) is not int or count < 0:
        raise RuntimeError('Candidate count must be a nonnegative integer')
    steps = [[]]
    steps += [[dict(index=i, negative=negative)]
              for i, negative in itertools.product(range(count + 2), (False, True))]
    steps += [[dict(index=0, negative=False), dict(index=1, negative=True)],
              [dict(index=0, negative=True), dict(index=1, negative=True)]]
    return [dict(steps=step, minimal=minimal, logging=logging)
            for step, minimal, logging in itertools.product(steps, (False, True), (False, True))]
