#[derive(Clone)]
pub struct SampleRecordExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::SampleRecord>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SampleRecordExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::SampleRecord>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::SampleRecord> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::SampleRecord> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::SampleRecord {
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

    pub fn get_quantity(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("quantity", |entity| entity.eval_quantity());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_zero_bound(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("zero_bound", |entity| entity.eval_zero_bound());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_negative_max(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("negative_max", |entity| entity.eval_negative_max());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_signed_max(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("signed_max", |entity| entity.eval_signed_max());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_wide_max(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("wide_max", |entity| entity.eval_wide_max());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_negative_bound(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("negative_bound", |entity| entity.eval_negative_bound());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_amount(self) -> crate::ValueExpression<'a, rust_decimal::Decimal> {
        let next = self.result.and_then("amount", |entity| entity.eval_amount());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_height(self) -> crate::ValueExpression<'a, f64> {
        let next = self.result.and_then("height", |entity| entity.eval_height());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_count(self) -> crate::ValueExpression<'a, Option<i64>> {
        let next = self.result.and_then("optional_count", |entity| entity.eval_optional_count());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_active(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("active", |entity| entity.eval_active());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_disabled(self) -> crate::ValueExpression<'a, bool> {
        let next = self.result.and_then("disabled", |entity| entity.eval_disabled());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_active(self) -> crate::ValueExpression<'a, Option<bool>> {
        let next = self.result.and_then("optional_active", |entity| entity.eval_optional_active());
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

    pub fn get_recorded_at(self) -> crate::ValueExpression<'a, teaql_core::time::Timestamp> {
        let next = self.result.and_then("recorded_at", |entity| entity.eval_recorded_at());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_recorded_at(self) -> crate::ValueExpression<'a, Option<teaql_core::time::Timestamp>> {
        let next = self.result.and_then("optional_recorded_at", |entity| entity.eval_optional_recorded_at());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_clock_text(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("clock_text", |entity| entity.eval_clock_text());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_clock_text(self) -> crate::ValueExpression<'a, Option<String>> {
        let next = self.result.and_then("optional_clock_text", |entity| entity.eval_optional_clock_text());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_ratio(self) -> crate::ValueExpression<'a, Option<f32>> {
        let next = self.result.and_then("ratio", |entity| entity.eval_ratio());
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
    pub fn get_first_links(self) -> crate::SampleLinkListExpression<'a> {
        let next = self.result.and_then("first_links", |entity| entity.eval_first_links());
        crate::SampleLinkListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_second_links(self) -> crate::SampleLinkListExpression<'a> {
        let next = self.result.and_then("second_links", |entity| entity.eval_second_links());
        crate::SampleLinkListExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct SampleRecordListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SampleRecord>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> SampleRecordListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::SampleRecord>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::SampleRecord>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::SampleRecord>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::SampleRecord> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::SampleRecordExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SampleRecordExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::SampleRecordExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::SampleRecordExpression::new(next, self.root_desc.clone())
    }
}