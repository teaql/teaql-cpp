#[derive(Clone)]
pub struct LabelExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Label>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> LabelExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Label>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Label> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Label> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Label {
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

    pub fn get_priority_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("priority_id", |entity| entity.eval_priority_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get_priority(self) -> crate::PriorityExpression<'a> {
        let next = self.result.and_then("priority", |entity| entity.eval_priority());
        crate::PriorityExpression::new(next, self.root_desc.clone())
    }
    pub fn priority_is_basic(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("priority_id", |entity| {
            if !entity.is_loaded("priority_id") {
                teaql_core::eval::EvalResult::NotLoaded { failed_node: "priority_id".to_string(), attempted_path: "priority_id".to_string() }
            } else {
                teaql_core::eval::EvalResult::Value(entity.priority_is_basic())
            }
        });
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn priority_is_extended(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("priority_id", |entity| {
            if !entity.is_loaded("priority_id") {
                teaql_core::eval::EvalResult::NotLoaded { failed_node: "priority_id".to_string(), attempted_path: "priority_id".to_string() }
            } else {
                teaql_core::eval::EvalResult::Value(entity.priority_is_extended())
            }
        });
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct LabelListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Label>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> LabelListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Label>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Label>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Label>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Label> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::LabelExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::LabelExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::LabelExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::LabelExpression::new(next, self.root_desc.clone())
    }
}