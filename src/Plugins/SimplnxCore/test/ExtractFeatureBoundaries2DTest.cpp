#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ExtractFeatureBoundaries2DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace extract_features_boundaries_2d_test
{
const DataPath k_ImageGeometryPath({"ImageGeometry"});
const DataPath k_FeatureIdsPath({"ImageGeometry", "Cell Data", "FeatureIds"});
const DataPath k_OutputEdgeGeometryPath({"Computed Feature Boundaries"});
const DataPath k_ExemplarEdgeGeometryPath({"Feature Boundaries"});

} // namespace extract_features_boundaries_2d_test

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter: 01_simple_adjacent", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz",
                                                              "ExtractFeatureBoundaries2D", true);

  std::vector<std::string> exemplarFilePaths = {"01_simple_adjacent.dream3d", "02_non_touching.dream3d",   "03_checkerboard.dream3d",  "04_single_feature.dream3d",
                                                "05_nested_features.dream3d", "06_corner_contact.dream3d", "07_complex_mixed.dream3d", "08_complex_mixed.dream3d"};

  for(const auto& fileName : exemplarFilePaths)
  {
    INFO(fmt::format("Input Data File: {}", fileName))
    {
      auto exemplarFilePath = fs::path(fmt::format("{}/ExtractFeatureBoundaries2D/{}", unit_test::k_TestFilesDir, fileName));

      // Read Exemplar DREAM3D File Filter
      // auto exemplarFilePath = "01_simple_adjacent.dream3d";
      DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

      // Instantiate the filter
      ExtractFeatureBoundaries2DFilter filter;
      Arguments args;

      // Set filter parameters
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_ImageGeometryPath));
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_FeatureIdsPath));
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath));
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ZValueChoice_Key, std::make_any<uint64>(0)); // Use min z value
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_CustomZValue_Key, std::make_any<float32>(0.0f));
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ExtractVirtualSampleEdges_Key, std::make_any<bool>(true));

      // Preflight the filter
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      // Execute the filter
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
      UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/test_output_{}", unit_test::k_BinaryTestOutputDir, fileName)));
#endif

      // Verify the output edge geometry was created
      const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath);
      REQUIRE(edgeGeom != nullptr);
      REQUIRE(edgeGeom->getNumberOfEdges() > 0);
      REQUIRE(edgeGeom->getNumberOfVertices() > 0);

      // Compare Geometries
      {
        const auto* exemplarGeom = dataStructure.getDataAs<EdgeGeom>(extract_features_boundaries_2d_test::k_ExemplarEdgeGeometryPath);
        REQUIRE(exemplarGeom != nullptr);
        UnitTest::CompareIGeometry(exemplarGeom, edgeGeom);
      }

      // Compare Shared Vertex List
      {
        const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Vertex List");
        const DataPath exemplarPath = extract_features_boundaries_2d_test::k_ExemplarEdgeGeometryPath.createChildPath("Shared Vertex List");
        UnitTest::CompareFloatArraysWithNans<float>(dataStructure, computedPath, exemplarPath);
      }

      // Compare Edge List
      {
        const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Edge List");
        const DataPath exemplarPath = extract_features_boundaries_2d_test::k_ExemplarEdgeGeometryPath.createChildPath("Shared Edge List");
        UnitTest::CompareArrays<uint64>(dataStructure, computedPath, exemplarPath);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter: Z Max", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz",
                                                              "ExtractFeatureBoundaries2D");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ExtractFeatureBoundaries2D/08_complex_mixed.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter
  ExtractFeatureBoundaries2DFilter filter;
  Arguments args;

  // Set filter parameters
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_ImageGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_FeatureIdsPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ZValueChoice_Key, std::make_any<uint64>(1)); // Use max z value
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_CustomZValue_Key, std::make_any<float32>(0.0f));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ExtractVirtualSampleEdges_Key, std::make_any<bool>(true));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Verify the output edge geometry was created
  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  REQUIRE(edgeGeom->getNumberOfEdges() > 0);
  REQUIRE(edgeGeom->getNumberOfVertices() > 0);

  const DataPath exemplarDataPath({"Feature Boundaries Max Z"});
  // Compare Geometries
  {
    const auto* exemplarGeom = dataStructure.getDataAs<EdgeGeom>(exemplarDataPath);
    UnitTest::CompareIGeometry(exemplarGeom, edgeGeom);
  }

  // Compare Shared Vertex List
  {
    const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Vertex List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Vertex List");
    UnitTest::CompareFloatArraysWithNans<float>(dataStructure, computedPath, exemplarPath);
  }

  // Compare Edge List
  {
    const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Edge List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Edge List");
    UnitTest::CompareArrays<uint64>(dataStructure, computedPath, exemplarPath);
  }
}

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter: Z Custom", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz",
                                                              "ExtractFeatureBoundaries2D");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ExtractFeatureBoundaries2D/08_complex_mixed.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter
  ExtractFeatureBoundaries2DFilter filter;
  Arguments args;

  // Set filter parameters
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_ImageGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_FeatureIdsPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ZValueChoice_Key, std::make_any<uint64>(2)); // Use custom z value
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_CustomZValue_Key, std::make_any<float32>(2.5f));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ExtractVirtualSampleEdges_Key, std::make_any<bool>(true));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Verify the output edge geometry was created
  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  REQUIRE(edgeGeom->getNumberOfEdges() > 0);
  REQUIRE(edgeGeom->getNumberOfVertices() > 0);

  const DataPath exemplarDataPath({"Feature Boundaries Custom Z"});
  // Compare Geometries
  {
    const auto* exemplarGeom = dataStructure.getDataAs<EdgeGeom>(exemplarDataPath);
    UnitTest::CompareIGeometry(exemplarGeom, edgeGeom);
  }

  // Compare Shared Vertex List
  {
    const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Vertex List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Vertex List");
    UnitTest::CompareFloatArraysWithNans<float>(dataStructure, computedPath, exemplarPath);
  }

  // Compare Edge List
  {
    const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Edge List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Edge List");
    UnitTest::CompareArrays<uint64>(dataStructure, computedPath, exemplarPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter: Z Min No Edges", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz",
                                                              "ExtractFeatureBoundaries2D");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ExtractFeatureBoundaries2D/08_complex_mixed.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter
  ExtractFeatureBoundaries2DFilter filter;
  Arguments args;

  // Set filter parameters
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_ImageGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_FeatureIdsPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ZValueChoice_Key, std::make_any<uint64>(0)); // Use min z value
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_CustomZValue_Key, std::make_any<float32>(2.5f));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ExtractVirtualSampleEdges_Key, std::make_any<bool>(false)); // NO EDGES around virtual volume

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Verify the output edge geometry was created
  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  REQUIRE(edgeGeom->getNumberOfEdges() > 0);
  REQUIRE(edgeGeom->getNumberOfVertices() > 0);

  const DataPath exemplarDataPath({"Feature Boundaries Min Z No Edges"});
  // Compare Geometries
  {
    const auto* exemplarGeom = dataStructure.getDataAs<EdgeGeom>(exemplarDataPath);
    UnitTest::CompareIGeometry(exemplarGeom, edgeGeom);
  }

  // Compare Shared Vertex List
  {
    const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Vertex List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Vertex List");
    UnitTest::CompareFloatArraysWithNans<float>(dataStructure, computedPath, exemplarPath);
  }

  // Compare Edge List
  {
    const DataPath computedPath = extract_features_boundaries_2d_test::k_OutputEdgeGeometryPath.createChildPath("Shared Edge List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Edge List");
    UnitTest::CompareArrays<uint64>(dataStructure, computedPath, exemplarPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
