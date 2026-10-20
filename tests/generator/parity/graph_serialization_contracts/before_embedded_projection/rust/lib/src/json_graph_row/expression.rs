#[derive(Clone)]
pub struct JsonGraphRowExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::JsonGraphRow>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> JsonGraphRowExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::JsonGraphRow>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::JsonGraphRow> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::JsonGraphRow> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::JsonGraphRow {
        self.resolve().expect("Relation was legitimately null in database!")
    }

    pub fn get_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("id", |entity| entity.eval_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_name(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("name", |entity| entity.eval_name());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_version(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("version", |entity| entity.eval_version());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("platform_id", |entity| entity.eval_platform_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_peer_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("peer_id", |entity| entity.eval_peer_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_single_peer_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("single_peer_id", |entity| entity.eval_single_peer_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get_peer(self) -> crate::JsonGraphRowExpression<'a> {
        let next = self.result.and_then("peer", |entity| entity.eval_peer());
        crate::JsonGraphRowExpression::new(next, self.root_desc.clone())
    }

    pub fn get_single_peer(self) -> crate::JsonGraphRowExpression<'a> {
        let next = self.result.and_then("single_peer", |entity| entity.eval_single_peer());
        crate::JsonGraphRowExpression::new(next, self.root_desc.clone())
    }
    pub fn get_single_child(self) -> crate::JsonGraphRowExpression<'a> {
        let next = self.result.and_then("single_child", |entity| entity.eval_single_child());
        crate::JsonGraphRowExpression::new(next, self.root_desc.clone())
    }
    pub fn get_child_rows(self) -> crate::JsonGraphRowListExpression<'a> {
        let next = self.result.and_then("child_rows", |entity| entity.eval_child_rows());
        crate::JsonGraphRowListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_parents(self) -> crate::SerializationParentListExpression<'a> {
        let next = self.result.and_then("parents", |entity| entity.eval_parents());
        crate::SerializationParentListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_boxed_parents(self) -> crate::SerializationParentListExpression<'a> {
        let next = self.result.and_then("boxed_parents", |entity| entity.eval_boxed_parents());
        crate::SerializationParentListExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct JsonGraphRowListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::JsonGraphRow>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> JsonGraphRowListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::JsonGraphRow>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::JsonGraphRow>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::JsonGraphRow>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::JsonGraphRow> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::JsonGraphRowExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::JsonGraphRowExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::JsonGraphRowExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::JsonGraphRowExpression::new(next, self.root_desc.clone())
    }
}