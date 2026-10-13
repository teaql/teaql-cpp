#[derive(Clone)]
pub struct ReadingExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Reading>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> ReadingExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Reading>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Reading> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Reading> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Reading {
        self.resolve().expect("Relation was legitimately null in database!")
    }

    pub fn get_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("id", |entity| entity.eval_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_text(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("text", |entity| entity.eval_text());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_active(self) -> crate::ValueExpression<'a, Option<bool>> {
        let next = self.result.and_then("active", |entity| entity.eval_active());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_balance(self) -> crate::ValueExpression<'a, Option<rust_decimal::Decimal>> {
        let next = self.result.and_then("balance", |entity| entity.eval_balance());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_height(self) -> crate::ValueExpression<'a, Option<f64>> {
        let next = self.result.and_then("height", |entity| entity.eval_height());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_ratio(self) -> crate::ValueExpression<'a, Option<f32>> {
        let next = self.result.and_then("ratio", |entity| entity.eval_ratio());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_count(self) -> crate::ValueExpression<'a, Option<i64>> {
        let next = self.result.and_then("count", |entity| entity.eval_count());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_day(self) -> crate::ValueExpression<'a, Option<chrono::NaiveDate>> {
        let next = self.result.and_then("day", |entity| entity.eval_day());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_recorded_at(self) -> crate::ValueExpression<'a, Option<teaql_core::time::Timestamp>> {
        let next = self.result.and_then("recorded_at", |entity| entity.eval_recorded_at());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_payload(self) -> crate::ValueExpression<'a, Option<serde_json::Value>> {
        let next = self.result.and_then("payload", |entity| entity.eval_payload());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_version(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("version", |entity| entity.eval_version());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct ReadingListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Reading>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> ReadingListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Reading>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Reading>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Reading>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Reading> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::ReadingExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::ReadingExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::ReadingExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::ReadingExpression::new(next, self.root_desc.clone())
    }
}