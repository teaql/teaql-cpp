
use teaql_runtime::EntityDataServiceBehavior;

#[derive(Clone, Debug, Default)]
pub struct HydrationProbeBehavior;

impl EntityDataServiceBehavior for HydrationProbeBehavior {}