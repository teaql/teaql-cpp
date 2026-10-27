"""Complete sample-field input recipe retained by the original collector."""

import itertools


def sample_field_inputs():
    clocks = [1735689600123, 1709208000456, 946684800789, 123]
    inputs = [dict(kind=kind, wall_millis=clock, root=root, scale='Tiny', seed=seed, rounds=1)
              for kind, clock, root, seed in itertools.product(
                  ['temporal', 'json_runtime'], clocks, [False, True],
                  [0, 7, 1094, 1095, 18446744073709551596])]
    inputs.extend(dict(kind='temporal', wall_millis=clock, root=True, scale='Tiny', seed=7, rounds=2)
                  for clock in clocks)
    inputs.append(dict(kind='temporal', wall_millis=1735689600123, root=True,
                       scale='Small', seed=1094, rounds=1))
    return inputs
