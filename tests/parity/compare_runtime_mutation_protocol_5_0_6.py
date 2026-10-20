"""Compare actual-source mutation errors, metadata order, batches and counters."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('runtime_mutation_protocol_5_0_6_cases.json.gz',
                         'Runtime mutation protocol 5.0.6'))
