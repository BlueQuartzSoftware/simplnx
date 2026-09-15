"""Test that simplnx_utilities generates valid, compilable Python code."""

import copy
import subprocess
import sys
import textwrap
import unittest

import simplnx as nx
import simplnx_test_dirs as nxtest
import simplnx_utilities


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

    def test_GeneratorImportsMissingPluginModuleOnDemand(self):
        script = textwrap.dedent("""
            import sys

            import simplnx as nx
            import simplnx_utilities

            nxor = sys.modules["orientationanalysis"]
            filter_type = nxor.ComputeIPFColorsFilter
            pipeline = nx.Pipeline()
            pipeline.append(filter_type(), filter_type.get_default_arguments())

            del sys.modules["orientationanalysis"]
            code = simplnx_utilities.generate_python_pipeline(pipeline)

            assert "orientationanalysis" in sys.modules
            assert "nxor.ComputeIPFColorsFilter.execute(" in code
        """)

        result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_PruneRegenerableStatsRemovesDerivedArraysWithoutChangingInput(self):
        stats = {
            "phases": [
                {
                    "phase_type": "Primary",
                    "axis_orientation": [1.0, 2.0],
                    "misorientation_bins": [3.0, 4.0],
                    "odf": [5.0, 6.0],
                    "distribution_sources": {"odf": "preset", "omega3": "user"},
                    "bin_numbers": [0.5, 1.5],
                    "feature_size_distribution": {"distribution_type": "LogNormal", "mu": [1.0]},
                    "preset_metadata": {"preset_name": "Primary Equiaxed", "preset_version": 1},
                },
                {
                    "phase_type": "Precipitate",
                    "axis_orientation": [7.0],
                    "misorientation_bins": [8.0],
                    "odf": [9.0],
                    "odf_weights": {
                        "euler1": [0.1],
                        "euler2": [0.2],
                        "euler3": [0.3],
                        "weights": [10.0],
                        "sigmas": [0.05],
                    },
                    "distribution_sources": {"odf": "user"},
                    "radial_distribution_function": {
                        "frequencies": [0.25, 0.75],
                        "min_distance": 1.0,
                        "max_distance": 2.0,
                    },
                },
            ]
        }
        original = copy.deepcopy(stats)

        actual = simplnx_utilities._prune_regenerable_stats(stats)

        self.assertEqual(stats, original)
        self.assertEqual(
            actual,
            {
                "phases": [
                    {
                        "phase_type": "Primary",
                        "distribution_sources": {"odf": "preset", "omega3": "user"},
                        "bin_numbers": [0.5, 1.5],
                        "feature_size_distribution": {"distribution_type": "LogNormal", "mu": [1.0]},
                        "preset_metadata": {"preset_name": "Primary Equiaxed", "preset_version": 1},
                    },
                    {
                        "phase_type": "Precipitate",
                        "odf_weights": {
                            "euler1": [0.1],
                            "euler2": [0.2],
                            "euler3": [0.3],
                            "weights": [10.0],
                            "sigmas": [0.05],
                        },
                        "distribution_sources": {"odf": "user"},
                        "radial_distribution_function": {
                            "frequencies": [0.25, 0.75],
                            "min_distance": 1.0,
                            "max_distance": 2.0,
                        },
                    },
                ]
            },
        )

    def test_PruneRegenerableStatsRetainsOnlyUserOdfWithoutWeights(self):
        user_bulk_odf = {
            "phases": [
                {
                    "phase_type": "Primary",
                    "distribution_sources": {"odf": "user"},
                    "odf_weights": {
                        "euler1": [],
                        "euler2": [],
                        "euler3": [],
                        "weights": [],
                        "sigmas": [],
                    },
                    "odf": [0.125, 0.375, 0.5],
                }
            ]
        }
        user_weighted_odf = {
            "phases": [
                {
                    "phase_type": "Primary",
                    "distribution_sources": {"odf": "user"},
                    "odf_weights": {
                        "euler1": [0.1],
                        "euler2": [0.2],
                        "euler3": [0.3],
                        "weights": [42.0],
                        "sigmas": [0.05],
                    },
                    "odf": [0.125, 0.375, 0.5],
                }
            ]
        }

        bulk_result = simplnx_utilities._prune_regenerable_stats(user_bulk_odf)
        weighted_result = simplnx_utilities._prune_regenerable_stats(user_weighted_odf)

        self.assertEqual(bulk_result["phases"][0]["odf"], [0.125, 0.375, 0.5])
        self.assertNotIn("odf", weighted_result["phases"][0])

    def test_EncodeStatsGeneratorUsesCompactFormatting(self):
        class LiteralStatsValue:
            def to_dict(self):
                return {"phases": [{"bin_numbers": list(range(20))}]}

        encoded = simplnx_utilities._encode_stats_generator(
            "stats_generator_data", LiteralStatsValue(), simplnx_utilities.CodeGenContext()
        )

        self.assertEqual(len("\n".join(encoded).splitlines()), 3)
        self.assertIn("[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,", encoded[0])

if __name__ == "__main__":
    unittest.main()
