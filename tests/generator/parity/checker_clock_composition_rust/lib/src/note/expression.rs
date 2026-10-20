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

    pub fn get_code(self) -> crate::ValueExpression<'a, String> {
        let next = self.result.and_then("code", |entity| entity.eval_code());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_count(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("count", |entity| entity.eval_count());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_payload(self) -> crate::ValueExpression<'a, serde_json::Value> {
        let next = self.result.and_then("payload", |entity| entity.eval_payload());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_version(self) -> crate::ValueExpression<'a, i64> {
        let next = self.result.and_then("version", |entity| entity.eval_version());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_owner_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("owner_id", |entity| entity.eval_owner_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_linked_id(self) -> crate::ValueExpression<'a, u64> {
        let next = self.result.and_then("linked_id", |entity| entity.eval_linked_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_linked_id(self) -> crate::ValueExpression<'a, Option<u64>> {
        let next = self.result.and_then("optional_linked_id", |entity| entity.eval_optional_linked_id());
        crate::ValueExpression::new(next, self.root_desc.clone())
    }
    pub fn get_owner(self) -> crate::TenantExpression<'a> {
        let next = self.result.and_then("owner", |entity| entity.eval_owner());
        crate::TenantExpression::new(next, self.root_desc.clone())
    }

    pub fn get_linked(self) -> crate::TenantExpression<'a> {
        let next = self.result.and_then("linked", |entity| entity.eval_linked());
        crate::TenantExpression::new(next, self.root_desc.clone())
    }

    pub fn get_optional_linked(self) -> crate::TenantExpression<'a> {
        let next = self.result.and_then("optional_linked", |entity| entity.eval_optional_linked());
        crate::TenantExpression::new(next, self.root_desc.clone())
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