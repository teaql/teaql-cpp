#[derive(Clone)]
pub struct CategoryExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Category>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> CategoryExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Category>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Category> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Category> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Category {
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

    pub fn get_parent_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("parent_id", |entity| entity.eval_parent_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get_parent(self) -> crate::CategoryExpression<'a> {
        let next = self.result.and_then("parent", |entity| entity.eval_parent());
        crate::CategoryExpression::new(next, self.root_desc.clone())
    }
    pub fn parent_is_c1(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("parent_id", |entity| {
            if !entity.is_loaded("parent_id") {
                teaql_core::eval::EvalResult::NotLoaded { failed_node: "parent_id".to_string(), attempted_path: "parent_id".to_string() }
            } else {
                teaql_core::eval::EvalResult::Value(entity.parent_is_c1())
            }
        });
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn parent_is_c22(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("parent_id", |entity| {
            if !entity.is_loaded("parent_id") {
                teaql_core::eval::EvalResult::NotLoaded { failed_node: "parent_id".to_string(), attempted_path: "parent_id".to_string() }
            } else {
                teaql_core::eval::EvalResult::Value(entity.parent_is_c22())
            }
        });
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_category_list(self) -> crate::CategoryListExpression<'a> {
        let next = self.result.and_then("category_list", |entity| entity.eval_category_list());
        crate::CategoryListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_status_list(self) -> crate::StatusListExpression<'a> {
        let next = self.result.and_then("status_list", |entity| entity.eval_status_list());
        crate::StatusListExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct CategoryListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Category>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> CategoryListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Category>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Category>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Category>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Category> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::CategoryExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::CategoryExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::CategoryExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::CategoryExpression::new(next, self.root_desc.clone())
    }
}