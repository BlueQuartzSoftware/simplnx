"""Test that simplnx_utilities generates valid, compilable Python code."""

import simplnx as nx
import simplnx_test_dirs as nxtest
import simplnx_utilities

# ---------------------------------------------------------------------------
# Test 1: Generate a full script from a pipeline with basic filters
# ---------------------------------------------------------------------------
print("=== Test 1: generate_python_pipeline with basic filters ===")

data_structure = nx.DataStructure()

pipeline = nx.Pipeline()
pipeline.append(nx.CreateImageGeometryFilter(), {
    "dimensions": [10, 10, 10],
    "origin": [0.0, 0.0, 0.0],
    "spacing": [1.0, 1.0, 1.0],
    "output_image_geometry_path": nx.DataPath("ImageGeom"),
    "cell_attribute_matrix_name": "CellData",
})

code = simplnx_utilities.generate_python_pipeline(pipeline)
print(code)

# Verify the code is syntactically valid Python
compile(code, "<generated_pipeline>", "exec")
print("Test 1 PASSED: generated code compiles successfully")

# ---------------------------------------------------------------------------
# Test 2: Generate filter snippets
# ---------------------------------------------------------------------------
print("\n=== Test 2: generate_python_filters ===")

filters = [pipeline[i] for i in range(len(pipeline))]
snippet = simplnx_utilities.generate_python_filters(filters)
print(snippet)

# Snippets reference data_structure and check_filter_result, so wrap them
# in a function body to make them compile without executing
wrapped = (
    "def _snippet(data_structure, check_filter_result):\n"
    + "\n".join("    " + line for line in snippet.splitlines())
)
compile(wrapped, "<generated_snippet>", "exec")
print("Test 2 PASSED: generated snippet compiles successfully")

# ---------------------------------------------------------------------------
# Test 3: Multi-filter pipeline with threshold parameter
# ---------------------------------------------------------------------------
print("\n=== Test 3: pipeline with ArrayThresholdSet parameter ===")

pipeline2 = nx.Pipeline()
pipeline2.append(nx.CreateImageGeometryFilter(), {
    "dimensions": [10, 10, 10],
    "origin": [0.0, 0.0, 0.0],
    "spacing": [1.0, 1.0, 1.0],
    "output_image_geometry_path": nx.DataPath("ImageGeom"),
    "cell_attribute_matrix_name": "CellData",
})
pipeline2.append(nx.CreateDataArrayFilter(), {
    "component_count": 1,
    "initialization_value_str": "0",
    "numeric_type_index": nx.NumericType.float32,
    "output_array_path": nx.DataPath("ImageGeom/CellData/Quality"),
    "tuple_dimensions": [[10, 10, 10]],
})

threshold = nx.ArrayThreshold()
threshold.array_path = nx.DataPath("ImageGeom/CellData/Quality")
threshold.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold.value = 0.5

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold]

pipeline2.append(nx.MultiThresholdObjectsFilter(), {
    "array_thresholds_object": threshold_set,
    "created_mask_type": nx.DataType.boolean,
    "output_data_array_name": "Mask",
})

code2 = simplnx_utilities.generate_python_pipeline(pipeline2)
print(code2)
compile(code2, "<generated_pipeline_thresholds>", "exec")
print("Test 3 PASSED: threshold pipeline code compiles successfully")

print("\n===> All pipeline_to_python tests passed")
