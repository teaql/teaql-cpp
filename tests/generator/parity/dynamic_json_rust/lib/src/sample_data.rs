
use std::collections::BTreeMap;
use crate::TeaqlRuntime;
use crate::Q;
use teaql_core::Entity as _;
use crate::request_support::AuditedSave as _;

pub trait IntoU64 {
    fn into_u64(self) -> u64;
}

impl IntoU64 for u64 {
    fn into_u64(self) -> u64 {
        self
    }
}

impl IntoU64 for Option<&teaql_core::Value> {
    fn into_u64(self) -> u64 {
        self.and_then(|v| v.try_u64()).unwrap_or_default()
    }
}

#[derive(Debug, Copy, Clone)]
pub enum SampleDataScale {
    Tiny,
    Small,
    Medium,
}

pub struct SampleDataPlan {
    pub scale: SampleDataScale,
    pub seed: u64,
}

impl SampleDataPlan {
    pub fn small() -> Self {
        Self {
            scale: SampleDataScale::Small,
            seed: 0,
        }
    }
}

pub struct SampleDataReport {
    pub generated: BTreeMap<&'static str, usize>,
    pub skipped: Vec<SampleDataSkipped>,
}

pub struct SampleDataSkipped {
    pub entity: &'static str,
    pub reason: String,
}

#[derive(Debug)]
pub struct SampleDataError {
    message: String,
}

impl SampleDataError {
    fn from_display(error: impl std::fmt::Display) -> Self {
        Self {
            message: error.to_string(),
        }
    }
}

impl std::fmt::Display for SampleDataError {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        formatter.write_str(&self.message)
    }
}

impl std::error::Error for SampleDataError {}

pub struct SampleDataState {
    pub plan: SampleDataPlan,
    pub references: BTreeMap<&'static str, Vec<u64>>,
    pub generated: BTreeMap<&'static str, usize>,
    pub skipped: Vec<SampleDataSkipped>,
}

impl SampleDataState {
    pub fn new(plan: SampleDataPlan) -> Self {
        Self {
            plan,
            references: BTreeMap::new(),
            generated: BTreeMap::new(),
            skipped: Vec::new(),
        }
    }

    pub fn add_reference(&mut self, entity: &'static str, id: u64) {
        self.references.entry(entity).or_default().push(id);
    }

    pub fn ids(&self, entity: &'static str) -> &[u64] {
        self.references.get(entity).map(|v| v.as_slice()).unwrap_or(&[])
    }

    pub fn pick_id(&self, entity: &'static str, salt: usize) -> Option<u64> {
        let ids = self.ids(entity);
        if ids.is_empty() {
            None
        } else {
            Some(ids[salt % ids.len()])
        }
    }

    pub fn pick_unused_id(&self, entity: &'static str, salt: usize, used: &std::collections::HashSet<u64>) -> Option<u64> {
        let ids = self.ids(entity);
        if ids.is_empty() {
            return None;
        }

        let best_id = ids[salt % ids.len()];
        if !used.contains(&best_id) {
            return Some(best_id);
        }

        for id in ids {
            if !used.contains(id) {
                return Some(*id);
            }
        }

        Some(best_id)
    }

    pub fn record_generated(&mut self, entity: &'static str) {
        *self.generated.entry(entity).or_default() += 1;
    }

    pub fn record_skipped(&mut self, entity: &'static str, reason: String) {
        self.skipped.push(SampleDataSkipped { entity, reason });
    }

    pub fn into_report(self) -> SampleDataReport {
        SampleDataReport {
            generated: self.generated,
            skipped: self.skipped,
        }
    }
}

pub async fn generate_sample_data<C>(
    context: &C,
    plan: SampleDataPlan,
) -> Result<SampleDataReport, SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
    log::info!("Starting sample data generation. Scale: {:?}, Seed: {}", plan.scale, plan.seed);
    let mut state = SampleDataState::new(plan);

    load_root_platforms(context, &mut state).await?; //depth: 0


    generate_companies(context, &mut state).await?;

    generate_people(context, &mut state).await?;

    generate_products(context, &mut state).await?;

    generate_orders(context, &mut state).await?;

    generate_profiles(context, &mut state).await?;

    generate_lines(context, &mut state).await?;


    let report = state.into_report();
    log::info!("Sample data generation completed successfully. Generated: {} tables, Skipped: {} tables.", report.generated.len(), report.skipped.len());
    Ok(report)
}

async fn load_root_platforms<C>(
    context: &C,
    state: &mut SampleDataState,
) -> Result<(), SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
    let list = Q::platforms().comment("what: inspect existing entities before sample-data initialization").purpose("why: avoid duplicate sample records").execute_for_list(context).await.unwrap_or_default();
    for item in list {
        state.add_reference(crate::Platform::ENTITY_NAME, item.id().into_u64());
    }
    Ok(())
}


async fn generate_companies<C>(
    context: &C,
    state: &mut SampleDataState,
) -> Result<(), SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
        if state.ids("platform").is_empty() {
            state.record_skipped(crate::Company::ENTITY_NAME, "Required dependency platform is missing in reference pool".to_string());
            log::info!("Skipped generating company: Required dependency platform is missing in reference pool.");
            return Ok(());
        }


    let object_fields_count = 0 + 1;
    let base_fanout = std::cmp::max(1, object_fields_count) * 20;

    let fanout = match state.plan.scale {
        SampleDataScale::Tiny => base_fanout,
        SampleDataScale::Small => base_fanout * 5,
        SampleDataScale::Medium => base_fanout * 50,
    };

    log::info!("Generating sample data for company (expected: {})...", fanout);

    for i in 0..fanout {
        let mut entity = Q::companies().comment("what: initialize a sample entity").purpose("why: populate the requested sample dataset").new_entity(context);
        let mut used_refs = std::collections::HashSet::<u64>::new();

                if let Some(ref_id) = state.pick_unused_id("platform", i as usize, &used_refs) {
                    entity.update_platform_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                entity.update_name(format!("{} {}", "string()", i + 1));



        let entity = entity.audit_as("Init Sample Data").save(context).await.map_err(SampleDataError::from_display)?;

        state.record_generated(crate::Company::ENTITY_NAME);

        if i % 20 == 0 {
            log::info!("Generating company: {}/{}", i, fanout);
        }

        state.add_reference(crate::Company::ENTITY_NAME, entity.id().into_u64());
    }

    log::info!("Successfully generated sample records for company.");
    Ok(())
}


async fn generate_people<C>(
    context: &C,
    state: &mut SampleDataState,
) -> Result<(), SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
        if state.ids("platform").is_empty() {
            state.record_skipped(crate::Person::ENTITY_NAME, "Required dependency platform is missing in reference pool".to_string());
            log::info!("Skipped generating person: Required dependency platform is missing in reference pool.");
            return Ok(());
        }


    let object_fields_count = 0 + 1;
    let base_fanout = std::cmp::max(1, object_fields_count) * 20;

    let fanout = match state.plan.scale {
        SampleDataScale::Tiny => base_fanout,
        SampleDataScale::Small => base_fanout * 5,
        SampleDataScale::Medium => base_fanout * 50,
    };

    log::info!("Generating sample data for person (expected: {})...", fanout);

    for i in 0..fanout {
        let mut entity = Q::people().comment("what: initialize a sample entity").purpose("why: populate the requested sample dataset").new_entity(context);
        let mut used_refs = std::collections::HashSet::<u64>::new();

                if let Some(ref_id) = state.pick_unused_id("platform", i as usize, &used_refs) {
                    entity.update_platform_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                entity.update_name(format!("{} {}", "string()", i + 1));

                entity.update_email(format!("{} {}", "string()", i + 1));



        let entity = entity.audit_as("Init Sample Data").save(context).await.map_err(SampleDataError::from_display)?;

        state.record_generated(crate::Person::ENTITY_NAME);

        if i % 20 == 0 {
            log::info!("Generating person: {}/{}", i, fanout);
        }

        state.add_reference(crate::Person::ENTITY_NAME, entity.id().into_u64());
    }

    log::info!("Successfully generated sample records for person.");
    Ok(())
}


async fn generate_products<C>(
    context: &C,
    state: &mut SampleDataState,
) -> Result<(), SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
        if state.ids("platform").is_empty() {
            state.record_skipped(crate::Product::ENTITY_NAME, "Required dependency platform is missing in reference pool".to_string());
            log::info!("Skipped generating product: Required dependency platform is missing in reference pool.");
            return Ok(());
        }


    let object_fields_count = 0 + 1;
    let base_fanout = std::cmp::max(1, object_fields_count) * 20;

    let fanout = match state.plan.scale {
        SampleDataScale::Tiny => base_fanout,
        SampleDataScale::Small => base_fanout * 5,
        SampleDataScale::Medium => base_fanout * 50,
    };

    log::info!("Generating sample data for product (expected: {})...", fanout);

    for i in 0..fanout {
        let mut entity = Q::products().comment("what: initialize a sample entity").purpose("why: populate the requested sample dataset").new_entity(context);
        let mut used_refs = std::collections::HashSet::<u64>::new();

                if let Some(ref_id) = state.pick_unused_id("platform", i as usize, &used_refs) {
                    entity.update_platform_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                entity.update_sku(format!("{} {}", "string()", i + 1));

                {
                    let max_val: u64 = "number()".parse().unwrap_or(1000);
                    let rand_val = (i as u64 + state.plan.seed) % max_val.max(1) + 1;
                    entity.update_price(rand_val as i64);
                }



        let entity = entity.audit_as("Init Sample Data").save(context).await.map_err(SampleDataError::from_display)?;

        state.record_generated(crate::Product::ENTITY_NAME);

        if i % 20 == 0 {
            log::info!("Generating product: {}/{}", i, fanout);
        }

        state.add_reference(crate::Product::ENTITY_NAME, entity.id().into_u64());
    }

    log::info!("Successfully generated sample records for product.");
    Ok(())
}


async fn generate_orders<C>(
    context: &C,
    state: &mut SampleDataState,
) -> Result<(), SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
        if state.ids("platform").is_empty() {
            state.record_skipped(crate::Order::ENTITY_NAME, "Required dependency platform is missing in reference pool".to_string());
            log::info!("Skipped generating order: Required dependency platform is missing in reference pool.");
            return Ok(());
        }

        if state.ids("person").is_empty() {
            state.record_skipped(crate::Order::ENTITY_NAME, "Required dependency person is missing in reference pool".to_string());
            log::info!("Skipped generating order: Required dependency person is missing in reference pool.");
            return Ok(());
        }

        if state.ids("company").is_empty() {
            state.record_skipped(crate::Order::ENTITY_NAME, "Required dependency company is missing in reference pool".to_string());
            log::info!("Skipped generating order: Required dependency company is missing in reference pool.");
            return Ok(());
        }


    let object_fields_count = 0 + 1 + 1 + 1;
    let base_fanout = std::cmp::max(1, object_fields_count) * 20;

    let fanout = match state.plan.scale {
        SampleDataScale::Tiny => base_fanout,
        SampleDataScale::Small => base_fanout * 5,
        SampleDataScale::Medium => base_fanout * 50,
    };

    log::info!("Generating sample data for order (expected: {})...", fanout);

    for i in 0..fanout {
        let mut entity = Q::orders().comment("what: initialize a sample entity").purpose("why: populate the requested sample dataset").new_entity(context);
        let mut used_refs = std::collections::HashSet::<u64>::new();

                if let Some(ref_id) = state.pick_unused_id("platform", i as usize, &used_refs) {
                    entity.update_platform_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                if let Some(ref_id) = state.pick_unused_id("person", i as usize, &used_refs) {
                    entity.update_customer_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                if let Some(ref_id) = state.pick_unused_id("company", i as usize, &used_refs) {
                    entity.update_seller_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                entity.update_code(format!("{} {}", "string()", i + 1));

                {
                    let max_val: u64 = "number()".parse().unwrap_or(1000);
                    let rand_val = (i as u64 + state.plan.seed) % max_val.max(1) + 1;
                    entity.update_amount(rand_val as i64);
                }

                {
                    let max_val: u64 = "integer()".parse().unwrap_or(1000);
                    let rand_val = (i as u64 + state.plan.seed) % max_val.max(1) + 1;
                    entity.update_priority(rand_val as i64);
                }



        let entity = entity.audit_as("Init Sample Data").save(context).await.map_err(SampleDataError::from_display)?;

        state.record_generated(crate::Order::ENTITY_NAME);

        if i % 20 == 0 {
            log::info!("Generating order: {}/{}", i, fanout);
        }

        state.add_reference(crate::Order::ENTITY_NAME, entity.id().into_u64());
    }

    log::info!("Successfully generated sample records for order.");
    Ok(())
}


async fn generate_profiles<C>(
    context: &C,
    state: &mut SampleDataState,
) -> Result<(), SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
        if state.ids("person").is_empty() {
            state.record_skipped(crate::Profile::ENTITY_NAME, "Required dependency person is missing in reference pool".to_string());
            log::info!("Skipped generating profile: Required dependency person is missing in reference pool.");
            return Ok(());
        }


    let object_fields_count = 0 + 1;
    let base_fanout = std::cmp::max(1, object_fields_count) * 20;

    let fanout = match state.plan.scale {
        SampleDataScale::Tiny => base_fanout,
        SampleDataScale::Small => base_fanout * 5,
        SampleDataScale::Medium => base_fanout * 50,
    };

    log::info!("Generating sample data for profile (expected: {})...", fanout);

    for i in 0..fanout {
        let mut entity = Q::profiles().comment("what: initialize a sample entity").purpose("why: populate the requested sample dataset").new_entity(context);
        let mut used_refs = std::collections::HashSet::<u64>::new();

                if let Some(ref_id) = state.pick_unused_id("person", i as usize, &used_refs) {
                    entity.update_person_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                entity.update_note(format!("{} {}", "string()", i + 1));



entity.audit_as("Init Sample Data").save(context).await.map_err(SampleDataError::from_display)?;

        state.record_generated(crate::Profile::ENTITY_NAME);

        if i % 20 == 0 {
            log::info!("Generating profile: {}/{}", i, fanout);
        }

    }

    log::info!("Successfully generated sample records for profile.");
    Ok(())
}


async fn generate_lines<C>(
    context: &C,
    state: &mut SampleDataState,
) -> Result<(), SampleDataError>
where
    C: TeaqlRuntime + ?Sized,
{
        if state.ids("order").is_empty() {
            state.record_skipped(crate::Line::ENTITY_NAME, "Required dependency order is missing in reference pool".to_string());
            log::info!("Skipped generating line: Required dependency order is missing in reference pool.");
            return Ok(());
        }

        if state.ids("product").is_empty() {
            state.record_skipped(crate::Line::ENTITY_NAME, "Required dependency product is missing in reference pool".to_string());
            log::info!("Skipped generating line: Required dependency product is missing in reference pool.");
            return Ok(());
        }


    let object_fields_count = 0 + 1 + 1;
    let base_fanout = std::cmp::max(1, object_fields_count) * 20;

    let fanout = match state.plan.scale {
        SampleDataScale::Tiny => base_fanout,
        SampleDataScale::Small => base_fanout * 5,
        SampleDataScale::Medium => base_fanout * 50,
    };

    log::info!("Generating sample data for line (expected: {})...", fanout);

    for i in 0..fanout {
        let mut entity = Q::lines().comment("what: initialize a sample entity").purpose("why: populate the requested sample dataset").new_entity(context);
        let mut used_refs = std::collections::HashSet::<u64>::new();

                if let Some(ref_id) = state.pick_unused_id("order", i as usize, &used_refs) {
                    entity.update_order_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                if let Some(ref_id) = state.pick_unused_id("product", i as usize, &used_refs) {
                    entity.update_product_id(ref_id);
                    used_refs.insert(ref_id);
                } else {
                    // Optional relation was missing in reference pool
                }
                {
                    let max_val: u64 = "integer()".parse().unwrap_or(1000);
                    let rand_val = (i as u64 + state.plan.seed) % max_val.max(1) + 1;
                    entity.update_quantity(rand_val as i64);
                }



entity.audit_as("Init Sample Data").save(context).await.map_err(SampleDataError::from_display)?;

        state.record_generated(crate::Line::ENTITY_NAME);

        if i % 20 == 0 {
            log::info!("Generating line: {}/{}", i, fanout);
        }

    }

    log::info!("Successfully generated sample records for line.");
    Ok(())
}
