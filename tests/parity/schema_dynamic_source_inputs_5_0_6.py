"""Ordered schema phases, failures and suspension/cancellation budgets."""
import itertools


def cases():
    schedules = [(0, 0, []), (0, 0, [0, 0]), (1, 2, [1, 1])]
    return [dict(provider=provider, dynamic=dynamic, business=business, fail=fail,
                 provider_delay=p, dynamic_delay=d, delays=callbacks, polls=polls)
            for provider, dynamic, business, fail, (p, d, callbacks), polls
            in itertools.product((False, True), (False, True), (False, True),
                                 ('none', 'provider', 'dynamic', 'business', '0', '1'),
                                 schedules, (0, 1, 2, 3, 4, 8))]
