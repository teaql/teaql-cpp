"""Check that changing intent retains independently configured privacy."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('mutation_intent_privacy_5_0_6_cases.json.gz', 'Mutation intent/privacy 5.0.6'))
