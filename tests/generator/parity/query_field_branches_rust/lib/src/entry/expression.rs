#[derive(Clone)]
pub struct EntryExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Entry>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> EntryExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Entry>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Entry> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Entry> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Entry {
        self.resolve().expect("Relation was legitimately null in database!")
    }

    pub fn get_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("id", |entity| entity.eval_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_label(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("label", |entity| entity.eval_label());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_label(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("optional_label", |entity| entity.eval_optional_label());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_notes(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("notes", |entity| entity.eval_notes());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_secret(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("secret", |entity| entity.eval_secret());
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

    pub fn get_amount(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("amount", |entity| entity.eval_amount());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_amount(self) -> crate::ValueExpression<'a, Option<rust_decimal::Decimal>> {
        let next = self.result.and_then("optional_amount", |entity| entity.eval_optional_amount());
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

    pub fn get_counter(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("counter", |entity| entity.eval_counter());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_day(self) -> crate::ValueExpression<'a, chrono::NaiveDate> {
        let next = self.result.and_then("day", |entity| entity.eval_day());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_day(self) -> crate::ValueExpression<'a, Option<chrono::NaiveDate>> {
        let next = self.result.and_then("optional_day", |entity| entity.eval_optional_day());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_moment(self) -> crate::ValueExpression<'a, teaql_core::time::Timestamp> {
        let next = self.result.and_then("moment", |entity| entity.eval_moment());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_moment(self) -> crate::ValueExpression<'a, Option<teaql_core::time::Timestamp>> {
        let next = self.result.and_then("optional_moment", |entity| entity.eval_optional_moment());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_version(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("version", |entity| entity.eval_version());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_tenant_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("tenant_id", |entity| entity.eval_tenant_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_tenant(self) -> crate::TenantExpression<'a> {
        let next = self.result.and_then("tenant", |entity| entity.eval_tenant());
        crate::TenantExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct EntryListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Entry>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> EntryListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Entry>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Entry>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Entry>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Entry> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::EntryExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::EntryExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::EntryExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::EntryExpression::new(next, self.root_desc.clone())
    }
}