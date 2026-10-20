"""Complete optional metadata/loaded-original/clone/frame diagnostic branches."""
from itertools import product


def cases():
    names = ('loaded', 'bound', 'row', 'active', 'copy', 'action')
    actions = ('none', 'set', 'delete', 'set_delete', 'delete_set', 'pop', 'clear', 'commit', 'mark_delete')
    return [dict(zip(names, row)) for row in product(range(3), *([[False, True]] * 4), actions)]
