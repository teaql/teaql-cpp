#[cfg(test)]
mod cpp_service_surface_observer {
    use super::*;

    fn same<T>(_: Option<T>, _: Option<T>) {}
    fn imports<'a, M: 'a, E: 'a>() {
        same(None::<EntityDataService<'a, E>>, None::<super::types::EntityDataService<'a, E>>);
        same(None::<ContextDataService<'a, E>>, None::<super::types::ContextDataService<'a, E>>);
        same(None::<RuntimeDataService<'a, M, E>>, None::<super::types::RuntimeDataService<'a, M, E>>);
        same(None::<UserContextMetadata<'a>>, None::<super::types::UserContextMetadata<'a>>);
        same(None::<RelationLoadPlan>, None::<super::types::RelationLoadPlan>);
        same(None::<InMemoryAggregationCache>, None::<super::cache::InMemoryAggregationCache>);
        same(None::<&dyn AggregationCacheBackend>, None::<&dyn super::cache::AggregationCacheBackend>);
        same(None::<GraphTransactionBoundary>, None::<super::executor::GraphTransactionBoundary>);
    }

    #[test]
    fn boundaries_and_exports() {
        imports::<crate::InMemoryMetadataStore, ()>();
        let values = [GraphTransactionBoundary::Started, GraphTransactionBoundary::AlreadyActive,
                      GraphTransactionBoundary::Unsupported];
        let names = ["Started", "AlreadyActive", "Unsupported"];
        for (index, value) in values.iter().enumerate() {
            let copied = *value;
            let cloned = value.clone();
            assert_eq!(copied, cloned);
            assert_eq!(format!("{cloned:?}"), names[index]);
            for (other_index, other) in values.iter().enumerate() {
                assert_eq!(cloned == *other, index == other_index);
            }
        }
        println!("SERVICE_BOUNDARIES=Started,AlreadyActive,Unsupported");
    }
}
