#[derive(Clone)]
pub struct PersonExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Person>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> PersonExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Person>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Person> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Person> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Person {
        self.resolve().expect("Relation was legitimately null in database!")
    }

    pub fn get_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("id", |entity| entity.eval_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_email(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("email", |entity| entity.eval_email());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_active(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("active", |entity| entity.eval_active());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_nickname(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("nickname", |entity| entity.eval_nickname());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_balance(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("balance", |entity| entity.eval_balance());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_height(self) -> crate::ValueExpression<'a, f64> {
        let next = self.result.and_then("height", |entity| entity.eval_height());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_ratio(self) -> crate::ValueExpression<'a, f32> {
        let next = self.result.and_then("ratio", |entity| entity.eval_ratio());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_age(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("age", |entity| entity.eval_age());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_birthday(self) -> crate::ValueExpression<'a, chrono::NaiveDate> {
        let next = self.result.and_then("birthday", |entity| entity.eval_birthday());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_updated_at(self) -> crate::ValueExpression<'a, teaql_core::time::Timestamp> {
        let next = self.result.and_then("updated_at", |entity| entity.eval_updated_at());
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
    pub fn get_device_list(self) -> crate::DeviceListExpression<'a> {
        let next = self.result.and_then("device_list", |entity| entity.eval_device_list());
        crate::DeviceListExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct PersonListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Person>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> PersonListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Person>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Person>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Person>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Person> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::PersonExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::PersonExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::PersonExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::PersonExpression::new(next, self.root_desc.clone())
    }
}