# Generated projection/relation shortcuts: Rust 5.0.6

Status: partial G04/G06/G13 evidence. This closes four named request API gaps;
it does not remove the default inheritance guard or complete generator parity.

Runtime: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Source authority: request/index.stg lines 1229-1271 and its selection helpers.

## Application contracts

| C++ method | Original behavior retained |
| --- | --- |
| selectSelfWithoutParent | Alias of selectSelfFields; includes inherited fields despite its name |
| selectAll | Select native fields, then forward and reverse to-one relations |
| selectChildren | selectAll plus reverse to-many collections |
| selectAny | Alias of selectChildren |

Adapters call existing native-field and relation selection methods. They retain
purpose/result types, Context binding and ref-qualified lvalue/rvalue chaining.
They do not infer descendant enhancement: enhanceChildrenIfNeeded remains its
separate source responsibility. Repeated calls append projections/relations in
source order, including duplicates. Assist explains these distinctions.

## Shared runtime discrepancy and repair

The additional SQLite tests found 42 differences after selecting a nested advisor
and then selectAll. The later identity-table reload replaced the advisor's loaded
nested Platform in C++; the original preserved an edge-owned exact view.

The original hydrate_compact_flat_plan installs a forward ancestor before its
descendants. After descendant hydration, it installs that edge's exact rows as
an option view, without replacing identity-table peers again. C++ now follows
that ordering through shared relation hydration/installation helpers. It retains
selected descendant availability, scope-specific projections, Context allocation
and existing graph replacement semantics. This is not a generic union of all
loaded fields or a policy/filter widening rule. Existing cancellation/wake
protocols continue to own asynchronous children.

## Evidence

The construction oracle adds 144 shortcut cases to the earlier 1,800, for
**1,944 matching observations**. Shortcut cases also compare ordered relation
trees rather than only native projections. They vary three inherited models,
normal/minimal/with-children entries, repeated shortcut application and recursive
enhancement. Full query facets/readonly-definition Debug remains outside that
explicit projection.

The SQLite graph oracle adds **216 matching observations**: 108 reads, 84 saves
and 24 optimistic conflicts. It observes explicitly selected root Platform and
reverse collection views as well as the earlier nested advisor state. The full
graph/save corpus now contains **2,313 matching observations**, preserving the
earlier forward/reverse cases. Original producer files remain unchanged.

Native tests additionally verify inherited-field alias behavior, duplicate
relation order, independent request copies, separation from child enhancement
and E traversal after the later identity reload. Existing explicit User/Global
retention and actual Context deletion checks exercise the resulting exact views.

Final selected native regression passes 9/9, including construction, graph/save,
result-type and dynamic-relation comparisons. Targeted ASan/UBSan passes 3/3 with
leak scanning disabled for the changed graph installation and nested lifetimes.
Module integration passes 2/2, selected Java tests 14/14 and all eight generated
artifact sets reproduce (88 files). Formatting checks 839 C++ files; active
source/path integrity checks pass. MIPS was not repeated.

The concrete remaining P2 gates are maintained in
[inheritance_qualification_matrix_5_0_6.md](inheritance_qualification_matrix_5_0_6.md).
