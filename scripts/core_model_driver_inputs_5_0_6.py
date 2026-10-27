"""Complete declared core-model stimuli; no expected-output selection."""
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DECLARED = {
    "graph_plan_scopes": [
        "tests/parity/graph_plan_scopes_5_0_6_cases.json.gz",
        "bfcaa39cb5ee34b99c0e5b6ae661ec6cfd407c9d3babd4bfe5700b4439f071ab"
    ],
    "scoped_comments": [
        "tests/parity/scoped_comments_5_0_6_cases.json",
        "42d60a45b193558aab626c12cfa7c213189823cde93d85554d2d1f849b67a7a1"
    ],
    "data_service_request_source": [
        "tests/parity/data_service_request_source_5_0_6_cases.json.gz",
        "94a1a3bb34dd46e4627a36ae2e632ac65372f048b929f027460c17c8a3fa859e"
    ],
    "data_service_model_scope": [
        "tests/parity/data_service_model_scope_baseline_5_0_6_cases.json.gz",
        "a201496e5eda2ccad0aceb7e18ffae5245c4bf91445c819260a35fcef207ec8e"
    ],
    "field_layout": [
        "tests/parity/field_layout_5_0_6_cases.json",
        "0e942bb6f3d931137d51652164c4899023788c398554b1b851a4855c7c96d78b"
    ],
    "loaded_snapshot": [
        "tests/parity/loaded_snapshot_5_0_6_cases.json",
        "28cc6cff90076bb1ccd5b0652469fc964b1e0daeb4fa7248ad05e7c1bf9e0868"
    ],
    "dynamic_provider": [
        "tests/parity/dynamic_provider_5_0_6_cases.json",
        "86126dc0cf161fea8f90dbf938426231ff3f3dba0d030bf1d9972c6824c312ac"
    ],
    "dynamic_carrier": [
        "tests/parity/dynamic_carrier_5_0_6_cases.json",
        "1e3e12723933b2b733680231001216e284efd92529f09a981413d2773cc3ee75"
    ],
    "original_snapshot": [
        "tests/parity/original_snapshot_5_0_6_cases.json",
        "1b3184515243022bde7dadcd6f3ddad1cf0bc468a7524ad4f6f4ae926052ab5a"
    ],
    "checker_provenance": [
        "tests/parity/checker_provenance_5_0_6_cases.json",
        "071965e3378ca6581f511ea6ba5f9cb19dacdda602b4d67cd13b9fb0af68eb58"
    ],
    "generated_relations": [
        "tests/parity/generated_relations_5_0_6_cases.json",
        "99c6f8efdf261681975094036df00a4bc8bcac967a8a1a3cc1fdc862a2748db1"
    ],
    "presentation": [
        "tests/parity/presentation_baseline_5_0_6_cases.json.gz",
        "81b04b2bc944b371534cdb8fae20eab79bc1b7cfa1d9007f7e687b8ca725ec45"
    ],
    "search": [
        "tests/parity/search_baseline_5_0_6_cases.json.gz",
        "23bfcc8578e1a8d0a44189a54860c39a705234ae8a08728710584a5c78594c8b"
    ],
    "value": [
        "tests/parity/value_5_0_6_cases.json.gz",
        "a206b16f4b12c852b90f4fec5b81d02a05b2c5dfdf35e254b144a824105b969b"
    ],
    "format": [
        "tests/parity/format_5_0_6_cases.json.gz",
        "543e0e8eed89ab19cc9c53bcc0dff2e4e70945cd3766953ee9548e0b5fff65e9"
    ],
    "presentation_api": [
        "tests/parity/presentation_api_baseline_5_0_6_cases.json.gz",
        "3fde1ce4d8c342a45e8cb397dcbf0eba003d56568370afa47dd1bc707abb8420"
    ],
    "core_graph_api": [
        "tests/parity/core_graph_api_baseline_5_0_6_cases.json.gz",
        "dc97ac4945fa1d054cc871a0ea00354438b1adea7ee788af480b5057a0edeaaa"
    ],
    "core_metadata_source": [
        "tests/parity/core_metadata_source_boundaries_5_0_6_cases.json.gz",
        "ad282f6ce3b6f5351ebf7b3c04b7041b76af1f8b52355d7350a3026d7e0ef4aa"
    ],
    "core_primitives": [
        "tests/parity/core_primitives_5_0_6_cases.json.gz",
        "007557e0c8e1f8f25cd6ae64511a5513da538e574ade55da707b88e03ff98d7e"
    ],
    "readonly_schema_hash_5_0_6": [
        "tests/parity/readonly_schema_hash_5_0_6_cases.json.gz",
        "e029c9633adb56230421e0f345c5d786cc6049aae2c14bec427fef8ac955ede3"
    ],
    "smart_list_api": [
        "tests/parity/smart_list_api_5_0_6_cases.json.gz",
        "dca45afb8d9101be4beb8d0f98dc4d01055d0ba70111eb399bfab0a63ff607cc"
    ],
    "core_shape_presentation": [
        "tests/parity/core_shape_presentation_5_0_6_cases.json",
        "5b2d2501b455408c8be76b6563a0fbd58c20f4d87e5522ada9f36da2ec6a5c5a"
    ],
    "compact_row_api": [
        "tests/parity/compact_row_api_5_0_6_cases.json.gz",
        "119ed4a2d4fb06af90162820a6d678b1977ff36fbf0a2e45b2adb8e19d2a3e03"
    ],
    "core_like_expression_source": [
        "tests/parity/core_like_expression_source_5_0_6_cases.json.gz",
        "130c8982ac4915b414b2c1d21be83b165dd7c86c754e26aa88ffc4a7b6026b84"
    ],
    "core_expression_boundaries": [
        "tests/parity/core_expression_boundaries_5_0_6_cases.json.gz",
        "e5319d9a489304adeaa0bd741d1e592a89c06d6ec10ca5205a2dfab00035a890"
    ],
    "safe_expression_empty_source": [
        "tests/parity/safe_expression_empty_source_5_0_6_cases.json.gz",
        "44bdfc27dcd163cf8531a5148c6fba6fb9f221833557deb2bbed3d399fa82509"
    ],
    "core_mutation_boundaries": [
        "tests/parity/core_mutation_boundaries_5_0_6_cases.json.gz",
        "96277051b04356751c0c1d3c78808ed521ac9849c7d9caba1f4c68642aa7a29a"
    ],
    "core_query_boundaries": [
        "tests/parity/core_query_boundaries_5_0_6_cases.json.gz",
        "01084ae1a950b094a497d5618f5a5986623c94c08a02aea66e3a840e6490089b"
    ],
    "core_query_intent_source": [
        "tests/parity/core_query_intent_source_5_0_6_cases.json.gz",
        "30958e5e1d3bdc2ca3a40f661fecdc3965e3b545d23f8add5237f35c64febf2a"
    ],
    "query_schema_builder": [
        "tests/parity/query_schema_builder_5_0_6_cases.json.gz",
        "3fc66150475bbcc8e0766ff68d81ae124f12806fc709b6c72d8f67b73991cc37"
    ],
    "query_full_state": [
        "tests/parity/query_full_state_5_0_6_cases.json.gz",
        "0847b404cafb6e4af17d51bd0dc826b946106dceb912e2fe68bad4290b9ed329"
    ],
    "row_extend_error": [
        "tests/parity/row_extend_error_5_0_6_cases.json.gz",
        "e928df100600676fae181e0894498817e113d22488749a6ae0fc7eb9ab9b6c0c"
    ],
    "dynamic_field_error_debug": [
        "tests/parity/dynamic_field_error_debug_5_0_6_cases.json.gz",
        "1e8de8bb213a2c285c13e68bc4fe0c7ad8d3f1c4de098af168961ea064a37a2a"
    ],
    "dynamic_search_clone": [
        "tests/parity/dynamic_search_clone_5_0_6_cases.json.gz",
        "1b70b6939c9bbf22b4cd064af5026a5a5bc78b71d1888c5d6431926dcb4e1afb"
    ]
}


def declared_inputs(name):
    filename, expected = DECLARED[name]
    path = ROOT / filename
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected
    data = json.loads(gzip.decompress(raw) if path.suffix == ".gz" else raw)
    return [row["input"] for row in data["cases"]]


def graph_plan_scopes_inputs():
    return declared_inputs('graph_plan_scopes')


def scoped_comments_inputs():
    return declared_inputs('scoped_comments')


def data_service_request_source_inputs():
    return declared_inputs('data_service_request_source')


def data_service_model_scope_inputs():
    return declared_inputs('data_service_model_scope')


def field_layout_inputs():
    return declared_inputs('field_layout')


def loaded_snapshot_inputs():
    return declared_inputs('loaded_snapshot')


def dynamic_provider_inputs():
    return declared_inputs('dynamic_provider')


def dynamic_carrier_inputs():
    return declared_inputs('dynamic_carrier')


def original_snapshot_inputs():
    return declared_inputs('original_snapshot')


def checker_provenance_inputs():
    return declared_inputs('checker_provenance')


def generated_relations_inputs():
    return declared_inputs('generated_relations')


def presentation_inputs():
    return declared_inputs('presentation')


def search_inputs():
    return declared_inputs('search')


def value_inputs():
    return declared_inputs('value')


def format_inputs():
    return declared_inputs('format')


def presentation_api_inputs():
    return declared_inputs('presentation_api')


def core_graph_api_inputs():
    return declared_inputs('core_graph_api')


def core_metadata_source_inputs():
    return declared_inputs('core_metadata_source')


def core_primitives_inputs():
    return declared_inputs('core_primitives')


def readonly_schema_hash_5_0_6_inputs():
    return declared_inputs('readonly_schema_hash_5_0_6')


def smart_list_api_inputs():
    return declared_inputs('smart_list_api')


def core_shape_presentation_inputs():
    return declared_inputs('core_shape_presentation')


def compact_row_api_inputs():
    return declared_inputs('compact_row_api')


def core_like_expression_source_inputs():
    return declared_inputs('core_like_expression_source')


def core_expression_boundaries_inputs():
    return declared_inputs('core_expression_boundaries')


def safe_expression_empty_source_inputs():
    return declared_inputs('safe_expression_empty_source')


def core_mutation_boundaries_inputs():
    return declared_inputs('core_mutation_boundaries')


def core_query_boundaries_inputs():
    return declared_inputs('core_query_boundaries')


def core_query_intent_source_inputs():
    return declared_inputs('core_query_intent_source')


def query_schema_builder_inputs():
    return declared_inputs('query_schema_builder')


def query_full_state_inputs():
    return declared_inputs('query_full_state')


def row_extend_error_inputs():
    return declared_inputs('row_extend_error')


def dynamic_field_error_debug_inputs():
    return declared_inputs('dynamic_field_error_debug')


def dynamic_search_clone_inputs():
    return declared_inputs('dynamic_search_clone')
