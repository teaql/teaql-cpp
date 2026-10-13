use teaql_core::Expr;

use crate::*;

pub struct PurposedQuery<T> {
    pub inner: T,
    pub purpose: String,
}

impl<T> PurposedQuery<T> {
    pub fn new(inner: T, purpose: impl Into<String>) -> Self {
        let purpose = purpose.into();
        assert!(!purpose.trim().is_empty(), "query purpose must not be empty");
        Self { inner, purpose }
    }
}

pub struct Q;

impl Q {
    pub fn platforms() -> PlatformRequest {
        PlatformRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn platforms_minimal() -> PlatformRequest {
        PlatformRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn platforms_with_children() -> PlatformRequest {
        PlatformRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn specialists() -> SpecialistRequest {
        SpecialistRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn specialists_minimal() -> SpecialistRequest {
        SpecialistRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn specialists_with_children() -> SpecialistRequest {
        SpecialistRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn schools() -> SchoolRequest {
        SchoolRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn schools_minimal() -> SchoolRequest {
        SchoolRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn schools_with_children() -> SchoolRequest {
        SchoolRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn academies() -> AcademyRequest {
        AcademyRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn academies_minimal() -> AcademyRequest {
        AcademyRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn academies_with_children() -> AcademyRequest {
        AcademyRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn research_academies() -> ResearchAcademyRequest {
        ResearchAcademyRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn research_academies_minimal() -> ResearchAcademyRequest {
        ResearchAcademyRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn research_academies_with_children() -> ResearchAcademyRequest {
        ResearchAcademyRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }



    pub fn enrollments() -> EnrollmentRequest {
        EnrollmentRequest::new()
            .select_self()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn enrollments_minimal() -> EnrollmentRequest {
        EnrollmentRequest::new()
            .and_filter(Expr::gt("version", 0_i64))
    }

    pub fn enrollments_with_children() -> EnrollmentRequest {
        EnrollmentRequest::new()
            .unlimited()
            .select_self_fields()
            .enhance_children_if_needed()
    }


}