"""Compare public mutation-policy review against the pinned Rust implementation."""
from compare_values import main
if __name__ == '__main__':
    raise SystemExit(main('mutation_policy_cases.json', 'Mutation policy'))
