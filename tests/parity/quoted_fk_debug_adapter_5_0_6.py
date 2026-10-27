"""Normalize only the approved generated Line foreign-key Debug descriptor."""

RUST_COLUMN = r'column_name: "\"order\""'
CPP_COLUMN = 'column_name: "order"'

DESCRIPTOR_PREFIX = 'PropertyDescriptor { name: "order_id", data_type: U64, nullable: false, '
DESCRIPTOR_SUFFIX = ', is_id: false, is_version: false, max_length: None'
RUST_DESCRIPTOR = DESCRIPTOR_PREFIX + RUST_COLUMN + DESCRIPTOR_SUFFIX
CPP_DESCRIPTOR = DESCRIPTOR_PREFIX + CPP_COLUMN + DESCRIPTOR_SUFFIX


def normalize_quoted_fk_descriptor(text):
    """Leave ordinary query text, SQL and values outside the exception."""
    return text.replace(RUST_DESCRIPTOR, CPP_DESCRIPTOR)
