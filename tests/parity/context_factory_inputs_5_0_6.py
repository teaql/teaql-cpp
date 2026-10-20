"""Factory controls only; all expected results come from the original methods."""
import itertools


def inputs():
    return [dict(metadata=metadata, described=described, registry=registry,
                 executor=executor, other_type=other_type, entity=entity)
            for metadata, described, registry, executor, other_type, entity in itertools.product(
                [False, True], [False, True], [False, True], ['none', 'plain', 'registered'],
                [False, True], ['Payment', 'Other'])]
