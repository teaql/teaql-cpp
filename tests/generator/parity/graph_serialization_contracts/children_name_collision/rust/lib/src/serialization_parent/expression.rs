#[derive(Clone)]
pub struct SerializationParentExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::SerializationParent>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SerializationParentExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::SerializationParent>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::SerializationParent> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::SerializationParent> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::SerializationParent {
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

    pub fn get_child_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("child_id", |entity| entity.eval_child_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_boxed_child_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("boxed_child_id", |entity| entity.eval_boxed_child_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get_child(self) -> crate::JsonGraphRowExpression<'a> {
        let next = self.result.and_then("child", |entity| entity.eval_child());
        crate::JsonGraphRowExpression::new(next, self.root_desc.clone())
    }

    pub fn get_boxed_child(self) -> crate::JsonGraphRowExpression<'a> {
        let next = self.result.and_then("boxed_child", |entity| entity.eval_boxed_child());
        crate::JsonGraphRowExpression::new(next, self.root_desc.clone())
    }
    pub fn get_children(self) -> crate::SerializationMemberListExpression<'a> {
        let next = self.result.and_then("children", |entity| entity.eval_children());
        crate::SerializationMemberListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_boxed_children(self) -> crate::SerializationMemberListExpression<'a> {
        let next = self.result.and_then("boxed_children", |entity| entity.eval_boxed_children());
        crate::SerializationMemberListExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct SerializationParentListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SerializationParent>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SerializationParentListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SerializationParent>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::SerializationParent>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::SerializationParent>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::SerializationParent> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::SerializationParentExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SerializationParentExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::SerializationParentExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SerializationParentExpression::new(next, self.root_desc.clone())
    }
}