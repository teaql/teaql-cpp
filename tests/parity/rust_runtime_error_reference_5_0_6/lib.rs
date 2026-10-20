//! Conditional contracts of the actual original generic error carrier.
//!
//! Display does not become available for a payload without Display:
//! ```compile_fail
//! use teaql_runtime::DataServiceError;
//! #[derive(Debug)]
//! struct Opaque;
//! let error = DataServiceError::Executor(Opaque);
//! let _ = error.to_string();
//! ```
//!
//! Debug does not become available merely because Display is implemented:
//! ```compile_fail
//! use std::fmt;
//! use teaql_runtime::DataServiceError;
//! struct DisplayOnly;
//! impl fmt::Display for DisplayOnly {
//!     fn fmt(&self, output: &mut fmt::Formatter<'_>) -> fmt::Result {
//!         output.write_str("display only")
//!     }
//! }
//! let _ = format!("{:?}", DataServiceError::Executor(DisplayOnly));
//! ```
//!
//! A String carrier supports Display/Debug but is not a standard Error:
//! ```compile_fail
//! use std::error::Error;
//! use teaql_runtime::DataServiceError;
//! fn require_error<T: Error>(_: &T) {}
//! require_error(&DataServiceError::Executor(String::from("text")));
//! ```
//!
//! The carrier's Error implementation requires a static executor payload:
//! ```compile_fail
//! use std::{error::Error, fmt};
//! use teaql_runtime::DataServiceError;
//! #[derive(Debug)]
//! struct Borrowed<'a>(&'a str);
//! impl fmt::Display for Borrowed<'_> {
//!     fn fmt(&self, output: &mut fmt::Formatter<'_>) -> fmt::Result {
//!         output.write_str(self.0)
//!     }
//! }
//! impl Error for Borrowed<'_> {}
//! fn require_error<T: Error>(_: &T) {}
//! let text = String::from("local");
//! let error = DataServiceError::Executor(Borrowed(&text));
//! require_error(&error);
//! ```

#[cfg(test)]
mod tests {
    use std::{error::Error, fmt};
    use teaql_runtime::DataServiceError;

    #[test]
    fn construction_does_not_require_display_debug_or_clone() {
        struct Opaque;
        let _ = DataServiceError::Executor(Opaque);
    }

    #[test]
    fn display_does_not_require_debug_or_clone() {
        struct DisplayOnly;
        impl fmt::Display for DisplayOnly {
            fn fmt(&self, output: &mut fmt::Formatter<'_>) -> fmt::Result {
                output.write_str("display only")
            }
        }
        assert_eq!(
            DataServiceError::Executor(DisplayOnly).to_string(),
            "display only"
        );
    }

    #[test]
    fn owned_error_does_not_require_clone() {
        #[derive(Debug)]
        struct OwnedError(String);
        impl fmt::Display for OwnedError {
            fn fmt(&self, output: &mut fmt::Formatter<'_>) -> fmt::Result {
                output.write_str(&self.0)
            }
        }
        impl Error for OwnedError {}
        fn require_error<T: Error>(value: &T) {
            assert!(value.source().is_none());
        }
        let error = DataServiceError::Executor(OwnedError(String::from("owned")));
        require_error(&error);
        assert_eq!(error.to_string(), "owned");
        assert_eq!(format!("{error:?}"), "Executor(OwnedError(\"owned\"))");
    }
}
