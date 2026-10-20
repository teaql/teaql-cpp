"""Retain entry controls and observe cursor-store side effects independently."""
from query_entry_protocol_inputs_5_0_6 import inputs as retained_inputs


def inputs():
    rows = retained_inputs()
    rows += [dict(case, cursor_probe=True) for case in rows
             if case.get('feature') in ('continuous_first', 'continuous_miss')]
    assert len(rows) == 8000
    return rows
