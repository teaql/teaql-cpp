// Access-only adapters appended to the complete original runtime in a disposable view.
impl EntityRuntimeState {
    #[doc(hidden)]
    pub fn __cpp_source_materialized(&self) -> bool { self.inner.get().is_some() }
    #[doc(hidden)]
    pub fn __cpp_source_error(&self) -> Option<LedgerCompositionError> {
        self.first_composition_error()
    }
    #[doc(hidden)]
    pub fn __cpp_source_compose(&self, source: &Self) -> Result<(), LedgerCompositionError> {
        self.adopt_mutations_from_checked(source)
    }
    #[doc(hidden)]
    pub fn __cpp_source_weak(source: &Self) -> Self { Self::fresh_with_weak_graph(source) }
}
