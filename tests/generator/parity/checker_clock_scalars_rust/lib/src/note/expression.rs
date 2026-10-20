#[derive(Clone)]
pub struct NoteExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Note>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> NoteExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Note>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Note> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Note> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Note {
        self.resolve().expect("Relation was legitimately null in database!")
    }

    pub fn get_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("id", |entity| entity.eval_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_text(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("text", |entity| entity.eval_text());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_text(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("optional_text", |entity| entity.eval_optional_text());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_code(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("code", |entity| entity.eval_code());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_code(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("optional_code", |entity| entity.eval_optional_code());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_integer(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("integer", |entity| entity.eval_integer());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_integer(self) -> crate::ValueExpression<'a, Option<i64>> {
        let next = self.result.and_then("optional_integer", |entity| entity.eval_optional_integer());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_long(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("long", |entity| entity.eval_long());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_long(self) -> crate::ValueExpression<'a, Option<i64>> {
        let next = self.result.and_then("optional_long", |entity| entity.eval_optional_long());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_decimal(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("decimal", |entity| entity.eval_decimal());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_decimal(self) -> crate::ValueExpression<'a, Option<rust_decimal::Decimal>> {
        let next = self.result.and_then("optional_decimal", |entity| entity.eval_optional_decimal());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_float(self) -> crate::ValueExpression<'a, f32> {
        let next = self.result.and_then("float", |entity| entity.eval_float());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_float(self) -> crate::ValueExpression<'a, Option<f32>> {
        let next = self.result.and_then("optional_float", |entity| entity.eval_optional_float());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_double(self) -> crate::ValueExpression<'a, f64> {
        let next = self.result.and_then("double", |entity| entity.eval_double());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_double(self) -> crate::ValueExpression<'a, Option<f64>> {
        let next = self.result.and_then("optional_double", |entity| entity.eval_optional_double());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_active(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("active", |entity| entity.eval_active());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_active(self) -> crate::ValueExpression<'a, Option<bool>> {
        let next = self.result.and_then("optional_active", |entity| entity.eval_optional_active());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_date(self) -> crate::ValueExpression<'a, chrono::NaiveDate> {
        let next = self.result.and_then("date", |entity| entity.eval_date());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_date(self) -> crate::ValueExpression<'a, Option<chrono::NaiveDate>> {
        let next = self.result.and_then("optional_date", |entity| entity.eval_optional_date());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_time(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("time", |entity| entity.eval_time());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_time(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("optional_time", |entity| entity.eval_optional_time());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_payload(self) -> crate::ValueExpression<'a, Option<serde_json::Value>> {
        let next = self.result.and_then("payload", |entity| entity.eval_payload());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_payload(self) -> crate::ValueExpression<'a, Option<serde_json::Value>> {
        let next = self.result.and_then("optional_payload", |entity| entity.eval_optional_payload());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_timestamp(self) -> crate::ValueExpression<'a, teaql_core::time::Timestamp> {
        let next = self.result.and_then("timestamp", |entity| entity.eval_timestamp());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_version(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("version", |entity| entity.eval_version());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct NoteListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Note>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> NoteListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Note>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Note>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Note>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Note> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::NoteExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::NoteExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::NoteExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::NoteExpression::new(next, self.root_desc.clone())
    }
}