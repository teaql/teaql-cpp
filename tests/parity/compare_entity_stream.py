"""Compare public entity-stream execution with original Rust calls."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('entity_stream_cases.json', 'Entity stream execution'))
