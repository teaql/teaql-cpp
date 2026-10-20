#[derive(Clone)]
pub struct PlatformExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a crate::Platform>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> PlatformExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a crate::Platform>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a crate::Platform> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a crate::Platform> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a crate::Platform {
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
    pub fn get_class_list(self) -> crate::ClassListExpression<'a> {
        let next = self.result.and_then("class_list", |entity| entity.eval_class_list());
        crate::ClassListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_operator_list(self) -> crate::OperatorListExpression<'a> {
        let next = self.result.and_then("operator_list", |entity| entity.eval_operator_list());
        crate::OperatorListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_template_list(self) -> crate::TemplateListExpression<'a> {
        let next = self.result.and_then("template_list", |entity| entity.eval_template_list());
        crate::TemplateListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_namespace_list(self) -> crate::NamespaceListExpression<'a> {
        let next = self.result.and_then("namespace_list", |entity| entity.eval_namespace_list());
        crate::NamespaceListExpression::new(next, self.root_desc.clone())
    }

    pub fn get_plain_record_list(self) -> crate::PlainRecordListExpression<'a> {
        let next = self.result.and_then("plain_record_list", |entity| entity.eval_plain_record_list());
        crate::PlainRecordListExpression::new(next, self.root_desc.clone())
    }
}

#[derive(Clone)]
pub struct PlatformListExpression<'a> {
    result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Platform>>,
    root_desc: std::sync::Arc<String>,
}

impl<'a> PlatformListExpression<'a> {
    pub fn new(result: teaql_core::eval::EvalResult<&'a teaql_core::SmartList<crate::Platform>>, root_desc: std::sync::Arc<String>) -> Self {
        Self { result, root_desc }
    }

    fn resolve(&self) -> Option<&'a teaql_core::SmartList<crate::Platform>> {
        match &self.result {
            teaql_core::eval::EvalResult::Value(v) => Some(*v),
            teaql_core::eval::EvalResult::Null => None,
            teaql_core::eval::EvalResult::NotLoaded { failed_node, attempted_path } => {
                crate::trigger_logic_bug_panic(&self.root_desc, &failed_node, &attempted_path)
            }
        }
    }

    pub fn eval(&self) -> Option<&'a teaql_core::SmartList<crate::Platform>> {
        self.resolve()
    }

    pub fn unwrap(&self) -> &'a teaql_core::SmartList<crate::Platform> {
        self.resolve().expect("List relation was legitimately null in database!")
    }

    pub fn size(&self) -> crate::ValueExpression<'a, usize> {
        let next = self.result.clone().and_then("size", |list| teaql_core::eval::EvalResult::Value(list.len()));
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn first(&self) -> crate::PlatformExpression<'a> {
        let next = self.result.clone().and_then("first", |list| {
            if let Some(item) = list.first() {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }

    pub fn get(&self, index: usize) -> crate::PlatformExpression<'a> {
        let next = self.result.clone().and_then("get", |list| {
            if let Some(item) = list.get(index) {
                teaql_core::eval::EvalResult::Value(item)
            } else {
                teaql_core::eval::EvalResult::Null
            }
        });
        crate::PlatformExpression::new(next, self.root_desc.clone())
    }
}