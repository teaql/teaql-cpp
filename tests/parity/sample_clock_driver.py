"""Run unmodified sample entries with one test-only process wall clock per group."""
import json
import os
import subprocess
from pathlib import Path
from compare_values import json_lines

def observe_fixed(executable, inputs, clock):
    groups = {}
    for index, value in enumerate(inputs):
        if type(value['wall_millis']) is not int:
            raise RuntimeError('Sample clock input requires integer wall milliseconds')
        groups.setdefault(value['wall_millis'], []).append((index, value))
    outputs = [None] * len(inputs)
    for millis, cases in groups.items():
        environment = dict(os.environ, LD_PRELOAD=str(Path(clock).resolve()),
                           TEAQL_TEST_SAMPLE_WALL_MILLIS=str(millis))
        result = subprocess.run([str(executable)],
                                input=''.join(json.dumps(value) + '\n' for _, value in cases),
                                text=True, capture_output=True, env=environment, check=True)
        values = json_lines(result.stdout)
        if len(values) != len(cases):
            raise RuntimeError('Sample clock probe returned incomplete observations')
        for (index, _), value in zip(cases, values):
            if type(value['wall_millis']) is not int or value['wall_millis'] != millis:
                raise RuntimeError(f'Process clock was not controlled: {value["wall_millis"]} != {millis}')
            outputs[index] = value
    return outputs
