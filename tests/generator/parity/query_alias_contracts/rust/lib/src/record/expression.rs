#[derive(Clone)]
pub struct RecordExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Record>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> RecordExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Record>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Record> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Record> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Record {
        self.resolve().expect("Relation was legitimately null in database!")
    }

    pub fn get_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("id", |entity| entity.eval_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_class(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("class", |entity| entity.eval_class());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_operator(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("operator", |entity| entity.eval_operator());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_template(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("template", |entity| entity.eval_template());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_type_field(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("type", |entity| entity.eval_type_field());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_match_field(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("match", |entity| entity.eval_match_field());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_ordinary(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("ordinary", |entity| entity.eval_ordinary());
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

    pub fn get_namespace_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("namespace_id", |entity| entity.eval_namespace_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_platform(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("platform", |entity| entity.eval_platform());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get_namespace(self) -> crate::PlatformExpression<'a> {
        let next = self.result.and_then("namespace", |entity| entity.eval_namespace());
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct RecordListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Record>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> RecordListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Record>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Record>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Record>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Record> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::RecordExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::RecordExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::RecordExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::RecordExpression::new(next, self.root_desc.clone())
    }
}