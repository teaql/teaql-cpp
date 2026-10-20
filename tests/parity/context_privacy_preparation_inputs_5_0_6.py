"""Independent graph-privacy preparation and intent-binding controls."""
import itertools


def inputs():
    return [dict(plan=plan, route=route, bind=bind, clear=clear)
            for plan, route, bind, clear in itertools.product(
                ['empty', 'secret'], ['context', 'bind_then_forward', 'forward_then_bind'],
                [False, True], [False, True])]
