#[derive(Clone)]
pub struct SampleLinkExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::SampleLink>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SampleLinkExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::SampleLink>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::SampleLink> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::SampleLink> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::SampleLink {
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
    pub fn get_first_record_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("first_record_id", |entity| entity.eval_first_record_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_second_record_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("second_record_id", |entity| entity.eval_second_record_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_first_record(self) -> crate::SampleRecordExpression<'a> {
        let next = self.result.and_then("first_record", |entity| entity.eval_first_record());
        crate::SampleRecordExpression::new(next, self.root_desc.clone())
    }

    pub fn get_second_record(self) -> crate::SampleRecordExpression<'a> {
        let next = self.result.and_then("second_record", |entity| entity.eval_second_record());
        crate::SampleRecordExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct SampleLinkListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SampleLink>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SampleLinkListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SampleLink>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::SampleLink>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::SampleLink>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::SampleLink> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::SampleLinkExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SampleLinkExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::SampleLinkExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SampleLinkExpression::new(next, self.root_desc.clone())
    }
}