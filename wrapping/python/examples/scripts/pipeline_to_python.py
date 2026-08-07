"""Test that simplnx_utilities generates valid, compilable Python code."""

import simplnx as nx
import simplnx_test_dirs as nxtest
import simplnx_utilities
import unittest

class PipelineConversionTest(unittest.TestCase):
    # ---------------------------------------------------------------------------
    # Test 1: Generate a full script from a pipeline with basic filters
    # ---------------------------------------------------------------------------
    def test_GenerateFullPipeline(self):
        print("=== Test 1: generate_python_pipeline with basic filters ===")

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
    def test_GenerateFilterSnippets(self):
        print("\n=== Test 2: generate_python_filters ===")

        pipeline = nx.Pipeline()
        pipeline.append(nx.CreateImageGeometryFilter(), {
            "dimensions": [10, 10, 10],
            "origin": [0.0, 0.0, 0.0],
            "spacing": [1.0, 1.0, 1.0],
            "output_image_geometry_path": nx.DataPath("ImageGeom"),
            "cell_attribute_matrix_name": "CellData",
        })

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
    def test_GenerateMultiFilterPipelineWithThresholdParameter(self):
        print("\n=== Test 3: pipeline with ArrayThresholdSet parameter ===")

        pipeline = nx.Pipeline()
        pipeline.append(nx.CreateImageGeometryFilter(), {
            "dimensions": [10, 10, 10],
            "origin": [0.0, 0.0, 0.0],
            "spacing": [1.0, 1.0, 1.0],
            "output_image_geometry_path": nx.DataPath("ImageGeom"),
            "cell_attribute_matrix_name": "CellData",
        })
        pipeline.append(nx.CreateDataArrayFilter(), {
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

        pipeline.append(nx.MultiThresholdObjectsFilter(), {
            "array_thresholds_object": threshold_set,
            "created_mask_type": nx.DataType.boolean,
            "output_data_array_name": "Mask",
        })

        code = simplnx_utilities.generate_python_pipeline(pipeline)
        print(code)
        compile(code, "<generated_pipeline_thresholds>", "exec")
        print("Test 3 PASSED: threshold pipeline code compiles successfully")

    def test_GenerateAllFilters(self):
        WIDTH = 80

        filter_list = nx.get_all_registered_filters()
        for nxfilter_type in filter_list:
            filter_name = nxfilter_type.name()
            with self.subTest(nxfilter=filter_name):
                print(f"======= Testing: {filter_name} ".ljust(WIDTH, "="))
                pipeline = nx.Pipeline()
                args = nxfilter_type.get_default_arguments()
                pipeline.append(nxfilter_type(), args)
                code = simplnx_utilities.generate_python_pipeline(pipeline)
                print(code)
                print("".ljust(WIDTH, "="))
                compile(code, "<string>", "exec")

if __name__ == "__main__":
    unittest.main()
