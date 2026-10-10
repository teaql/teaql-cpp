"""Expose unchanged private policy-plan helpers from an ignored locked-source archive."""
import sys
import build_graph_fix_oracle as base

BRIDGE = '''
#[doc(hidden)]
pub fn __teaql_cpp_graph_policy_plan(context: &UserContext, plan: &GraphMutationPlan)
    -> Result<MutationPlan, RuntimeError> {
    mutation_policy::graph_policy_plan(context, plan)
}
#[doc(hidden)]
pub fn __teaql_cpp_ledger_policy_plan(context: &UserContext, node: &GraphNode,
    root: &EntityRuntimeState) -> MutationPlan {
    mutation_policy::ledger_policy_plan(context, node, root)
}
'''

if __name__ == '__main__':
    base.EXTRA_EXPORTS = BRIDGE
    sys.argv += ['--oracle', str(base.ROOT / 'tests/parity/policy_plan_oracle.rs'),
                 '--generated-dir', str(base.ROOT / 'build/rust-policy-plan-original')]
    if '--target-dir' not in sys.argv:
        sys.argv += ['--target-dir', str(base.ROOT / 'build/rust-policy-plan-target')]
    base.main()
