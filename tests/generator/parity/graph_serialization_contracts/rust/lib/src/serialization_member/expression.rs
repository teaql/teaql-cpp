#[derive(Clone)]
pub struct SerializationMemberExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::SerializationMember>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SerializationMemberExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::SerializationMember>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::SerializationMember> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::SerializationMember> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::SerializationMember {
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

    pub fn get_parent_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("parent_id", |entity| entity.eval_parent_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_boxed_parent_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("boxed_parent_id", |entity| entity.eval_boxed_parent_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get_parent(self) -> crate::SerializationParentExpression<'a> {
        let next = self.result.and_then("parent", |entity| entity.eval_parent());
        crate::SerializationParentExpression::new(next, self.root_desc.clone())
    }

    pub fn get_boxed_parent(self) -> crate::SerializationParentExpression<'a> {
        let next = self.result.and_then("boxed_parent", |entity| entity.eval_boxed_parent());
        crate::SerializationParentExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct SerializationMemberListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SerializationMember>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SerializationMemberListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SerializationMember>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::SerializationMember>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::SerializationMember>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::SerializationMember> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::SerializationMemberExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SerializationMemberExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::SerializationMemberExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SerializationMemberExpression::new(next, self.root_desc.clone())
    }
}