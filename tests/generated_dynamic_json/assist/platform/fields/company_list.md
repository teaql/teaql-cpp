# Field Assist: platform.company_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalCompanyList()`; query view handle: `companyListHandle()`.
- Reverse relation: `companyList()` returns SmartList<Company>&; query `selectCompanyList()`, `selectCompanyListWith(CompanyRequest<P>)`, `withCompanyList(CompanyRequest<P>)`; expression `getCompanyList()`.

Presence: `haveCompanies()`, `haveNoCompanies()`. Matching: `withCompanyListMatching(child)`, `withoutCompanyListMatching(child)`.

Count: `countCompanies()`, `countCompaniesAs(alias)`, `countCompaniesWith(alias, child)`. Statistics/details: `statsFromCompanies(child)`, `statsFromCompaniesAs(alias, child)`, `groupByCompaniesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
