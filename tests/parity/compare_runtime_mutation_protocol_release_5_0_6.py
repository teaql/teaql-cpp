"""Compare isolated C++ wrapping counters with actual Release Rust methods."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('runtime_mutation_protocol_release_5_0_6_cases.json.gz',
                         'Runtime mutation isolated Release counter 5.0.6'))
