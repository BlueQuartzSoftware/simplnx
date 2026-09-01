#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ExtractFeatureBoundaries2DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;
using namespace nx::core;
namespace
{

const DataPath k_ImageGeometryPath({"ImageGeometry"});
const DataPath k_FeatureIdsPath({"ImageGeometry", "Cell Data", "FeatureIds"});
const DataPath k_OutputEdgeGeometryPath({"Computed Feature Boundaries"});
const DataPath k_ExemplarEdgeGeometryPath({"Feature Boundaries"});

// This filter requires a single Z plane. A 2048x2048 plane exercises 4,194,304 cells,
// comparable to a large 3D benchmark without allocating untouched Z slices.
constexpr usize k_BenchmarkDimX = 2048;
constexpr usize k_BenchmarkDimY = 2048;
constexpr usize k_BenchmarkCellCount = k_BenchmarkDimX * k_BenchmarkDimY;
constexpr usize k_BenchmarkMidX = k_BenchmarkDimX / 2;
constexpr usize k_BenchmarkMidY = k_BenchmarkDimY / 2;
constexpr float32 k_BenchmarkOriginX = 10.0F;
constexpr float32 k_BenchmarkOriginY = -20.0F;
constexpr float32 k_BenchmarkSpacingX = 0.5F;
constexpr float32 k_BenchmarkSpacingY = 0.25F;
constexpr float32 k_BenchmarkZ = 7.5F;

void BuildBenchmarkInput(DataStructure& dataStructure)
{
  const ShapeType cellTupleShape = {1, k_BenchmarkDimY, k_BenchmarkDimX};

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({k_BenchmarkDimX, k_BenchmarkDimY, 1});
  imageGeom->setOrigin({k_BenchmarkOriginX, k_BenchmarkOriginY, 5.0F});
  imageGeom->setSpacing({k_BenchmarkSpacingX, k_BenchmarkSpacingY, 2.0F});

  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = Int32Array::Create(dataStructure, "FeatureIds", featureIdsStore, cellData->getId());
  REQUIRE(featureIds != nullptr);

  auto rowBuffer = std::make_unique<int32[]>(k_BenchmarkDimX);
  for(usize y = 0; y < k_BenchmarkDimY; y++)
  {
    const int32 yRegion = y < k_BenchmarkMidY ? 0 : 2;
    for(usize x = 0; x < k_BenchmarkDimX; x++)
    {
      rowBuffer[x] = 1 + yRegion + (x < k_BenchmarkMidX ? 0 : 1);
    }

    const Result<> writeResult = featureIdsStore->copyFromBuffer(y * k_BenchmarkDimX, nonstd::span<const int32>(rowBuffer.get(), k_BenchmarkDimX));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
}

void RequireEdgeCoordinates(const EdgeGeom& edgeGeom, usize edgeIndex, const std::array<float32, 3>& expectedStart, const std::array<float32, 3>& expectedEnd)
{
  const auto& edges = edgeGeom.getEdgesRef();
  const auto& vertices = edgeGeom.getVerticesRef();
  REQUIRE(edgeIndex < edgeGeom.getNumberOfEdges());

  const usize startVertex = edges[edgeIndex * 2];
  const usize endVertex = edges[edgeIndex * 2 + 1];
  for(usize component = 0; component < 3; component++)
  {
    REQUIRE(vertices[startVertex * 3 + component] == expectedStart[component]);
    REQUIRE(vertices[endVertex * 3 + component] == expectedEnd[component]);
  }
}

/**
 * @brief Verifies the computed Edge Geometry against the exemplar Edge Geometry.
 * @param dataStructure Contains computed and exemplar geometries.
 * @param exemplarDataPath Exemplar EdgeGeom path.
 * @param computedDataPath Computed EdgeGeom path.
 */
void VerifyOutput(const DataStructure& dataStructure, const DataPath& exemplarDataPath, const DataPath& computedDataPath)
{
  // The filter must create the output edge geometry.
  const auto* computedEdgeGeom = dataStructure.getDataAs<EdgeGeom>(computedDataPath);
  REQUIRE(computedEdgeGeom != nullptr);

  // const DataPath exemplarDataPath({"Feature Boundaries Max Z"});
  // Compare geometry metadata and arrays.
  {
    const auto* exemplarGeom = dataStructure.getDataAs<EdgeGeom>(exemplarDataPath);
    REQUIRE(exemplarGeom != nullptr);
    UnitTest::CompareIGeometry(exemplarGeom, computedEdgeGeom);
  }

  // Compare Shared Vertex List
  {
    const DataPath computedPath = k_OutputEdgeGeometryPath.createChildPath("Shared Vertex List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Vertex List");
    UnitTest::CompareFloatArraysWithNans<float>(dataStructure, computedPath, exemplarPath);
  }

  // Compare Edge List
  {
    const DataPath computedPath = k_OutputEdgeGeometryPath.createChildPath("Shared Edge List");
    const DataPath exemplarPath = exemplarDataPath.createChildPath("Shared Edge List");
    UnitTest::CompareArrays<uint64>(dataStructure, computedPath, exemplarPath);
  }
}

} // namespace

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter::Valid_8_Cases", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz", "ExtractFeatureBoundaries2D", true);

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
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeometryPath));
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
      args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(k_OutputEdgeGeometryPath));
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

      VerifyOutput(dataStructure, k_ExemplarEdgeGeometryPath, k_OutputEdgeGeometryPath);
      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter: Z Max", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz", "ExtractFeatureBoundaries2D");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ExtractFeatureBoundaries2D/08_complex_mixed.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter
  ExtractFeatureBoundaries2DFilter filter;
  Arguments args;

  // Set filter parameters
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(k_OutputEdgeGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ZValueChoice_Key, std::make_any<uint64>(1)); // Use max z value
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
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/test_output_08_complex_mixed_z_max", unit_test::k_BinaryTestOutputDir)));
#endif

  VerifyOutput(dataStructure, DataPath({"Feature Boundaries Max Z"}), k_OutputEdgeGeometryPath);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter: Z Custom", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz", "ExtractFeatureBoundaries2D");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ExtractFeatureBoundaries2D/08_complex_mixed.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter
  ExtractFeatureBoundaries2DFilter filter;
  Arguments args;

  // Set filter parameters
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(k_OutputEdgeGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ZValueChoice_Key, std::make_any<uint64>(2)); // Use custom z value
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_CustomZValue_Key, std::make_any<float32>(2.5f));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ExtractVirtualSampleEdges_Key, std::make_any<bool>(true));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/test_output_08_complex_mixed_z_custom", unit_test::k_BinaryTestOutputDir)));
#endif

  VerifyOutput(dataStructure, DataPath({"Feature Boundaries Custom Z"}), k_OutputEdgeGeometryPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractFeatureBoundaries2DFilter: Z Min No Edges", "[SimplnxCore][ExtractFeatureBoundaries2DFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ExtractFeatureBoundaries2D.tar.gz", "ExtractFeatureBoundaries2D");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ExtractFeatureBoundaries2D/08_complex_mixed.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter
  ExtractFeatureBoundaries2DFilter filter;
  Arguments args;

  // Set filter parameters
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_InputImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(k_OutputEdgeGeometryPath));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ZValueChoice_Key, std::make_any<uint64>(0)); // Use min z value
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_CustomZValue_Key, std::make_any<float32>(2.5f));
  args.insertOrAssign(ExtractFeatureBoundaries2DFilter::k_ExtractVirtualSampleEdges_Key, std::make_any<bool>(false)); // NO EDGES around virtual volume

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/test_output_08_complex_mixed_no_edges", unit_test::k_BinaryTestOutputDir)));
#endif

  VerifyOutput(dataStructure, DataPath({"Feature Boundaries Min Z No Edges"}), k_OutputEdgeGeometryPath);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
