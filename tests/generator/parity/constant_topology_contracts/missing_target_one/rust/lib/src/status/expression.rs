#[derive(Clone)]
pub struct StatusExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Status>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> StatusExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Status>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Status> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Status> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Status {
        self.resolve().expect("Relation was legitimately null in database!")
    }

    pub fn get_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("id", |entity| entity.eval_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_code(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("code", |entity| entity.eval_code());
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

    pub fn get_category_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("category_id", |entity| entity.eval_category_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get_category(self) -> crate::CategoryExpression<'a> {
        let next = self.result.and_then("category", |entity| entity.eval_category());
        crate::CategoryExpression::new(next, self.root_desc.clone())
    }
    pub fn category_is_c22(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("category_id", |entity| {
            if !entity.is_loaded("category_id") {
                teaql_core::eval::EvalResult::NotLoaded { failed_node: "category_id".to_string(), attempted_path: "category_id".to_string() }
            } else {
                teaql_core::eval::EvalResult::Value(entity.category_is_c22())
            }
        });
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn category_is_c33(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("category_id", |entity| {
            if !entity.is_loaded("category_id") {
                teaql_core::eval::EvalResult::NotLoaded { failed_node: "category_id".to_string(), attempted_path: "category_id".to_string() }
            } else {
                teaql_core::eval::EvalResult::Value(entity.category_is_c33())
            }
        });
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct StatusListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Status>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> StatusListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Status>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Status>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Status>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Status> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::StatusExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::StatusExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::StatusExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::StatusExpression::new(next, self.root_desc.clone())
    }
}