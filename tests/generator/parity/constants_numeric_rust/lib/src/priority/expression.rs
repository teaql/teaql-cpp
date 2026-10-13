#[derive(Clone)]
pub struct PriorityExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Priority>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> PriorityExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Priority>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Priority> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Priority> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Priority {
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

    pub fn get_code(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("code", |entity| entity.eval_code());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_rank(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("rank", |entity| entity.eval_rank());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_weight(self) -> crate::ValueExpression<'a, f64> {
        let next = self.result.and_then("weight", |entity| entity.eval_weight());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_ratio(self) -> crate::ValueExpression<'a, f32> {
        let next = self.result.and_then("ratio", |entity| entity.eval_ratio());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_cost(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("cost", |entity| entity.eval_cost());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_day(self) -> crate::ValueExpression<'a, chrono::NaiveDate> {
        let next = self.result.and_then("day", |entity| entity.eval_day());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_note(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("note", |entity| entity.eval_note());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_rank(self) -> crate::ValueExpression<'a, Option<i64>> {
        let next = self.result.and_then("optional_rank", |entity| entity.eval_optional_rank());
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
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }
    pub fn get_label_list(self) -> crate::LabelListExpression<'a> {
        let next = self.result.and_then("label_list", |entity| entity.eval_label_list());
        crate::LabelListExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct PriorityListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Priority>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> PriorityListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Priority>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Priority>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Priority>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Priority> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::PriorityExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::PriorityExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::PriorityExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::PriorityExpression::new(next, self.root_desc.clone())
    }
}