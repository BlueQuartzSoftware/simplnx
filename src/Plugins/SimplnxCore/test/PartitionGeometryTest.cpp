/**
 * @file PartitionGeometryTest.cpp
 * @brief Tests each partitioning mode, supported geometry, error path, and OOC algorithm path.
 */

#include <catch2/catch.hpp>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <memory>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/PartitionGeometryFilter.hpp"
#include "SimplnxCore/Filters/ReadDREAM3DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const fs::path k_TestFilesPath = fs::path(unit_test::k_DREAM3DDataDir.str()) / fs::path("TestFiles") / fs::path("PartitionGeometryTest");
const fs::path k_ImageGeomTestFilePath = k_TestFilesPath / fs::path("image_geom.dream3d");
const fs::path k_RectGridGeomTestFilePath = k_TestFilesPath / fs::path("rectgrid_geom.dream3d");
const fs::path k_EdgeGeomTestFilePath = k_TestFilesPath / fs::path("edge_geom.dream3d");
const fs::path k_HexahedralGeomTestFilePath = k_TestFilesPath / fs::path("hexahedral_geom.dream3d");
const fs::path k_QuadGeomTestFilePath = k_TestFilesPath / fs::path("quad_geom.dream3d");
const fs::path k_TetrahedralGeomTestFilePath = k_TestFilesPath / fs::path("tetrahedral_geom.dream3d");
const fs::path k_TriangleGeomTestFilePath = k_TestFilesPath / fs::path("triangle_geom.dream3d");
const fs::path k_VertexGeomTestFilePath = k_TestFilesPath / fs::path("vertex_geom.dream3d");
const fs::path k_PlanalXYNodeGeomTestFilePath = k_TestFilesPath / fs::path("planal_xy_node_geom.dream3d");
const fs::path k_PlanalXZNodeGeomTestFilePath = k_TestFilesPath / fs::path("planal_xz_node_geom.dream3d");
const fs::path k_PlanalYZNodeGeomTestFilePath = k_TestFilesPath / fs::path("planal_yz_node_geom.dream3d");

constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkPartitionsPerAxis = 20;
constexpr usize k_BenchmarkCellsPerPartition = k_BenchmarkDim / k_BenchmarkPartitionsPerAxis;
constexpr usize k_BenchmarkSliceTuples = k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkTotalTuples = k_BenchmarkSliceTuples * k_BenchmarkDim;
constexpr usize k_BenchmarkFeatureCount = k_BenchmarkPartitionsPerAxis * k_BenchmarkPartitionsPerAxis * k_BenchmarkPartitionsPerAxis;
constexpr int32 k_BenchmarkStartingFeatureId = 7;
const std::string k_BenchmarkGeomName = "Partition Benchmark Geometry";
const std::string k_BenchmarkCellDataName = "Cell Data";
const std::string k_BenchmarkInputArrayName = "Input Values";
const std::string k_BenchmarkPartitionIdsName = "Partition Ids";
const std::string k_BenchmarkPartitionDataName = "Partition Data";
const std::string k_BenchmarkPartitionGridName = "Partition Grid";
const std::string k_BenchmarkPartitionGridCellDataName = "Cell Data";
const std::string k_BenchmarkPartitionGridFeatureIdsName = "Feature Ids";
const DataPath k_BenchmarkGeomPath({k_BenchmarkGeomName});
const DataPath k_BenchmarkCellDataPath = k_BenchmarkGeomPath.createChildPath(k_BenchmarkCellDataName);
const DataPath k_BenchmarkInputArrayPath = k_BenchmarkCellDataPath.createChildPath(k_BenchmarkInputArrayName);
const DataPath k_BenchmarkPartitionIdsPath = k_BenchmarkCellDataPath.createChildPath(k_BenchmarkPartitionIdsName);
const DataPath k_BenchmarkPartitionGridPath({k_BenchmarkPartitionGridName});
const DataPath k_BenchmarkPartitionGridFeatureIdsPath = k_BenchmarkPartitionGridPath.createChildPath(k_BenchmarkPartitionGridCellDataName).createChildPath(k_BenchmarkPartitionGridFeatureIdsName);

/**
 * @brief Builds a 200-cubed ImageGeom with sequential cell values for timing tests.
 * @param dataStructure Receives the geometry and input array.
 */
void BuildPartitionGeometryBenchmarkInput(DataStructure& dataStructure)
{
  const ShapeType cellTupleShape = {k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim};
  auto* imageGeom = ImageGeom::Create(dataStructure, k_BenchmarkGeomName);
  imageGeom->setDimensions({k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});

  auto* cellData = AttributeMatrix::Create(dataStructure, k_BenchmarkCellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto inputStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_BenchmarkInputArrayPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* inputArray = Int32Array::Create(dataStructure, k_BenchmarkInputArrayName, inputStore, cellData->getId());
  auto& inputStoreRef = inputArray->getDataStoreRef();
  auto sliceBuffer = std::make_unique<int32[]>(k_BenchmarkSliceTuples);
  for(usize z = 0; z < k_BenchmarkDim; z++)
  {
    const usize sliceOffset = z * k_BenchmarkSliceTuples;
    for(usize index = 0; index < k_BenchmarkSliceTuples; index++)
    {
      sliceBuffer[index] = static_cast<int32>(sliceOffset + index);
    }

    const Result<> writeResult = inputStoreRef.copyFromBuffer(sliceOffset, nonstd::span<const int32>(sliceBuffer.get(), k_BenchmarkSliceTuples));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
}

/**
 * @brief Calculates the expected partition identifier for one benchmark cell.
 * @param x Zero-based X cell coordinate.
 * @param y Zero-based Y cell coordinate.
 * @param z Zero-based Z cell coordinate.
 * @return Expected identifier with the configured starting offset.
 */
constexpr int32 ExpectedPartitionId(usize x, usize y, usize z)
{
  const usize partitionX = x / k_BenchmarkCellsPerPartition;
  const usize partitionY = y / k_BenchmarkCellsPerPartition;
  const usize partitionZ = z / k_BenchmarkCellsPerPartition;
  return k_BenchmarkStartingFeatureId + static_cast<int32>(partitionX + (partitionY * k_BenchmarkPartitionsPerAxis) + (partitionZ * k_BenchmarkPartitionsPerAxis * k_BenchmarkPartitionsPerAxis));
}

/**
 * @brief Creates arguments for basic partitioning by partition count.
 * @param inputGeometryPath Geometry to partition.
 * @param attrMatrixPath AttributeMatrix that receives partition identifiers.
 * @param partitionIdsArrayName Output identifier array name.
 * @param numOfPartitionsPerAxis Partition counts in {X, Y, Z} order.
 * @param maskArrayPath Optional vertex mask path.
 * @return Configured basic-mode arguments.
 */
Arguments createBasicPartitionGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName, const IntVec3& numOfPartitionsPerAxis,
                                                const std::optional<DataPath>& maskArrayPath)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Basic));
  args.insert(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, std::vector<int32>{numOfPartitionsPerAxis.getX(), numOfPartitionsPerAxis.getY(), numOfPartitionsPerAxis.getZ()});
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);

  if(maskArrayPath.has_value())
  {
    args.insert(PartitionGeometryFilter::k_UseVertexMask_Key, true);
    args.insert(PartitionGeometryFilter::k_VertexMaskPath_Key, *maskArrayPath);
  }

  return args;
}

/**
 * @brief Creates arguments for an explicit regular partition grid.
 * @param inputGeometryPath Geometry to partition.
 * @param attrMatrixPath AttributeMatrix that receives partition identifiers.
 * @param partitionIdsArrayName Output identifier array name.
 * @param numOfPartitionsPerAxis Partition counts in {X, Y, Z} order.
 * @param partitioningSchemeOrigin Partition-grid origin in geometry units.
 * @param lengthPerPartition Partition lengths in geometry units.
 * @return Configured advanced-mode arguments.
 */
Arguments createAdvancedPartitionGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName, const IntVec3& numOfPartitionsPerAxis,
                                                   const FloatVec3& partitioningSchemeOrigin, const FloatVec3& lengthPerPartition)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Advanced));
  args.insert(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, std::vector<int32>{numOfPartitionsPerAxis.getX(), numOfPartitionsPerAxis.getY(), numOfPartitionsPerAxis.getZ()});
  args.insert(PartitionGeometryFilter::k_PartitionGridOrigin_Key, std::vector<float>{partitioningSchemeOrigin.getX(), partitioningSchemeOrigin.getY(), partitioningSchemeOrigin.getZ()});
  args.insert(PartitionGeometryFilter::k_CellLength_Key, std::vector<float>{lengthPerPartition.getX(), lengthPerPartition.getY(), lengthPerPartition.getZ()});
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);
  return args;
}

/**
 * @brief Creates arguments for partitioning within an explicit bounding box.
 * @param inputGeometryPath Geometry to partition.
 * @param attrMatrixPath AttributeMatrix that receives partition identifiers.
 * @param partitionIdsArrayName Output identifier array name.
 * @param numOfPartitionsPerAxis Partition counts in {X, Y, Z} order.
 * @param lowerLeftCoord Minimum grid coordinates in geometry units.
 * @param upperRightCoord Maximum grid coordinates in geometry units.
 * @return Configured bounding-box-mode arguments.
 */
Arguments createBoundingBoxPartitionGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName,
                                                      const IntVec3& numOfPartitionsPerAxis, const FloatVec3& lowerLeftCoord, const FloatVec3& upperRightCoord)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::BoundingBox));
  args.insert(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, std::vector<int32>{numOfPartitionsPerAxis.getX(), numOfPartitionsPerAxis.getY(), numOfPartitionsPerAxis.getZ()});
  args.insert(PartitionGeometryFilter::k_MinGridCoord_Key, std::vector<float>{lowerLeftCoord.getX(), lowerLeftCoord.getY(), lowerLeftCoord.getZ()});
  args.insert(PartitionGeometryFilter::k_MaxGridCoord_Key, std::vector<float>{upperRightCoord.getX(), upperRightCoord.getY(), upperRightCoord.getZ()});
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);

  return args;
}

/**
 * @brief Creates arguments that use an existing partition grid.
 * @param inputGeometryPath Geometry to partition.
 * @param attrMatrixPath AttributeMatrix that receives partition identifiers.
 * @param partitionIdsArrayName Output identifier array name.
 * @param existingPSPath Existing partition-grid geometry path.
 * @return Configured existing-grid-mode arguments.
 */
Arguments createExistingPartitioningSchemeGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName, const DataPath& existingPSPath)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid));
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_ExistingPartitionGridPath_Key, existingPSPath);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);
  return args;
}

using FileSentinelType = nx::core::UnitTest::TestFileSentinel;
using SharedFileSentinelType = std::shared_ptr<FileSentinelType>;
// One shared sentinel keeps the extracted geometry fixtures available across each parameterized loop.
SharedFileSentinelType s_FileSentinel;
} // namespace

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Basic", "[Plugins][PartitionGeometryFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  const std::string partitionIdsArrayName = "PartitioningSchemeIds";
  const DataPath existingPSGeometryPath = {{"ExemplaryPSDataContainer"}};

  std::vector<std::filesystem::path> filePaths = {k_ImageGeomTestFilePath,       k_RectGridGeomTestFilePath,    k_TriangleGeomTestFilePath,   k_TriangleGeomTestFilePath,  k_EdgeGeomTestFilePath,
                                                  k_EdgeGeomTestFilePath,        k_VertexGeomTestFilePath,      k_VertexGeomTestFilePath,     k_QuadGeomTestFilePath,      k_QuadGeomTestFilePath,
                                                  k_TetrahedralGeomTestFilePath, k_TetrahedralGeomTestFilePath, k_HexahedralGeomTestFilePath, k_HexahedralGeomTestFilePath};
  std::vector<IntVec3> partitionDimensions = {{5, 5, 5},   {5, 5, 5},  {5, 4, 4},  {5, 4, 4},    {4, 4, 4},    {4, 4, 4}, {20, 10, 5},
                                              {20, 10, 5}, {10, 5, 3}, {10, 5, 3}, {100, 45, 8}, {100, 45, 8}, {6, 7, 8}, {6, 7, 8}};
  std::vector<std::string> amNames = {"CellData",   "CellData",   "VertexData", "VertexData", "VertexData", "VertexData", "VertexData",
                                      "VertexData", "VertexData", "VertexData", "VertexData", "VertexData", "VertexData", "VertexData"};
  std::vector<std::string> maskArrayNames = {"", "", "", "Mask", "", "Mask", "", "Mask", "", "Mask", "", "Mask", "", "Mask"};
  std::vector<std::string> exemplaryArrayNames = {"ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds",       "ExemplaryPartitioningSchemeIds", "MaskedExemplaryPartitioningSchemeIds",
                                                  "ExemplaryPartitioningSchemeIds", "MaskedExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "MaskedExemplaryPartitioningSchemeIds",
                                                  "ExemplaryPartitioningSchemeIds", "MaskedExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "MaskedExemplaryPartitioningSchemeIds",
                                                  "ExemplaryPartitioningSchemeIds", "MaskedExemplaryPartitioningSchemeIds"};
  size_t lastIndex = 13;
  size_t index = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);

  SECTION("BasicPartitionArguments")
  {
    // Parallel parameter vectors must describe the same number of geometry scenarios.
    REQUIRE(filePaths.size() > index);
    REQUIRE(partitionDimensions.size() > index);
    REQUIRE(amNames.size() > index);
    REQUIRE(maskArrayNames.size() > index);
    REQUIRE(exemplaryArrayNames.size() > index);

    // The first geometry acquires the shared extracted fixture directory.
    if(index == 0)
    {
      s_FileSentinel = std::make_shared<FileSentinelType>(nx::core::unit_test::k_TestFilesDir, "PartitionGeometryTest.tar.gz", "PartitionGeometryTest");
    }

    std::cout << "Basic Partition Arguments: " << filePaths[index] << std::endl;
    const IntVec3 numOfPartitionsPerAxis = partitionDimensions[index];
    const DataPath inputGeometryPath = {{"DataContainer"}};
    DataPath attrMatrixPath = {{"DataContainer", amNames[index]}};

    DataStructure dataStructure;
    {
      const ReadDREAM3DFilter importD3DFilter;
      Arguments importD3DArgs;
      importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(filePaths[index]));
      // Preflight must accept the selected geometry and partition mode.
      auto executeResult = importD3DFilter.execute(dataStructure, importD3DArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

    {
      DataPath maskPath;
      std::optional<DataPath> optMaskPath = std::nullopt;
      if(!maskArrayNames[index].empty())
      {
        maskPath = DataPath({"DataContainer", amNames[index], "Mask"});
        optMaskPath = {maskPath};
      }
      Arguments partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, optMaskPath);

      const PartitionGeometryFilter filter;
      // Execution must create partition identifiers for the selected geometry.
      auto executeResult = scope.executeFilter(filter, dataStructure, partitionGeometryArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      attrMatrixPath = partitionGeometryArgs.value<DataPath>(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key);
    }

    const Int32Array& partitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(partitionIdsArrayName));
    const Int32Array& exemplaryPartitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(exemplaryArrayNames[index]));

    REQUIRE(partitionIds.getSize() == exemplaryPartitionIds.getSize());

    const AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();
    const AbstractDataStore<int32>& exemplaryPartitionIdsStore = exemplaryPartitionIds.getDataStoreRef();
    for(size_t i = 0; i < partitionIds.getSize(); i++)
    {
      const int32_t partitionId = partitionIdsStore[i];
      const int32_t exemplaryId = exemplaryPartitionIdsStore[i];
      REQUIRE(partitionId == exemplaryId);
    }

    // The final geometry releases and removes the shared extracted fixtures.
    if(index == lastIndex)
    {
      s_FileSentinel = nullptr;
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Advanced", "[Plugins][PartitionGeometryFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  const std::string partitionIdsArrayName = "PartitioningSchemeIds";
  const DataPath existingPSGeometryPath = {{"ExemplaryPSDataContainer"}};

  std::vector<std::filesystem::path> filePaths = {k_ImageGeomTestFilePath,  k_RectGridGeomTestFilePath, k_TriangleGeomTestFilePath,    k_EdgeGeomTestFilePath,
                                                  k_VertexGeomTestFilePath, k_QuadGeomTestFilePath,     k_TetrahedralGeomTestFilePath, k_HexahedralGeomTestFilePath};
  std::vector<IntVec3> partitionDimensions = {{5, 5, 5}, {5, 5, 5}, {5, 4, 4}, {4, 4, 4}, {20, 10, 5}, {10, 5, 3}, {100, 45, 8}, {6, 7, 8}};
  std::vector<FloatVec3> partitionOrigins = {{-10, 5, 2},
                                             {0, 0, 0},
                                             {-0.997462, -0.997462, -0.00001},
                                             {-0.997462, -0.997462, -0.00001},
                                             {-0.997462, -0.997462, -0.00001},
                                             {-0.997462, -0.997462, -0.00001},
                                             {-0.997462, -0.997462, -0.00001},
                                             {0.9999989867210388, 0.9999989867210388, 1.5499989986419678}};
  std::vector<FloatVec3> partitionSpacing = {{5, 5, 5},
                                             {6, 6, 6},
                                             {0.398984, 0.49873, 0.247939},
                                             {0.49873, 0.49873, 0.247939},
                                             {0.099746, 0.199492, 0.198351},
                                             {0.199492, 0.398984, 0.330585333333333},
                                             {0.0199492, 0.044331555555556, 0.12397},
                                             {1.105000376701355, 0.2857145667076111, 0.2500002384185791}};
  std::vector<std::string> amNames = {"CellData", "CellData", "VertexData", "VertexData", "VertexData", "VertexData", "VertexData", "VertexData"};
  // std::vector<std::string> maskArrayNames = {"", "", "", "Mask", "", "Mask", "", "Mask", "", "Mask", "", "Mask", "", "Mask"};
  std::vector<std::string> exemplaryArrayNames = {"ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds",
                                                  "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds"};
  size_t lastIndex = 7;
  size_t index = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);

  SECTION("BasicPartitionArguments")
  {
    // Parallel parameter vectors must describe the same number of geometry scenarios.
    REQUIRE(filePaths.size() == lastIndex + 1);
    REQUIRE(partitionDimensions.size() == lastIndex + 1);
    REQUIRE(amNames.size() == lastIndex + 1);
    //  REQUIRE(maskArrayNames.size() == lastIndex + 1);
    REQUIRE(exemplaryArrayNames.size() == lastIndex + 1);
    REQUIRE(partitionOrigins.size() == lastIndex + 1);
    REQUIRE(partitionSpacing.size() == lastIndex + 1);

    // The first geometry acquires the shared extracted fixture directory.
    if(index == 0)
    {
      s_FileSentinel = std::make_shared<FileSentinelType>(nx::core::unit_test::k_TestFilesDir, "PartitionGeometryTest.tar.gz", "PartitionGeometryTest");
    }

    std::cout << "Basic Partition Arguments: " << filePaths[index] << std::endl;
    const IntVec3 numOfPartitionsPerAxis = partitionDimensions[index];
    const DataPath inputGeometryPath = {{"DataContainer"}};
    DataPath attrMatrixPath = {{"DataContainer", amNames[index]}};

    DataStructure dataStructure;
    {
      const ReadDREAM3DFilter importD3DFilter;
      Arguments importD3DArgs;
      importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(filePaths[index]));
      // Execute the filter and check result
      auto executeResult = importD3DFilter.execute(dataStructure, importD3DArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

    {
      Arguments partitionGeometryArgs =
          createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitionOrigins[index], partitionSpacing[index]);

      const PartitionGeometryFilter filter;
      // Execution must create partition identifiers for the selected geometry.
      auto executeResult = scope.executeFilter(filter, dataStructure, partitionGeometryArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      attrMatrixPath = partitionGeometryArgs.value<DataPath>(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key);
    }

    const Int32Array& partitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(partitionIdsArrayName));
    const Int32Array& exemplaryPartitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(exemplaryArrayNames[index]));

    REQUIRE(partitionIds.getSize() == exemplaryPartitionIds.getSize());

    const AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();
    const AbstractDataStore<int32>& exemplaryPartitionIdsStore = exemplaryPartitionIds.getDataStoreRef();
    for(size_t i = 0; i < partitionIds.getSize(); i++)
    {
      const int32_t partitionId = partitionIdsStore[i];
      const int32_t exemplaryId = exemplaryPartitionIdsStore[i];
      REQUIRE(partitionId == exemplaryId);
    }

    // The final geometry releases and removes the shared extracted fixtures.
    if(index == lastIndex)
    {
      s_FileSentinel = nullptr;
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Bounding Box", "[Plugins][PartitionGeometryFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  const std::string partitionIdsArrayName = "PartitioningSchemeIds";
  const DataPath existingPSGeometryPath = {{"ExemplaryPSDataContainer"}};

  std::vector<std::filesystem::path> filePaths = {k_ImageGeomTestFilePath,  k_RectGridGeomTestFilePath, k_TriangleGeomTestFilePath,    k_EdgeGeomTestFilePath,
                                                  k_VertexGeomTestFilePath, k_QuadGeomTestFilePath,     k_TetrahedralGeomTestFilePath, k_HexahedralGeomTestFilePath};
  std::vector<IntVec3> partitionDimensions = {{5, 5, 5}, {5, 5, 5}, {5, 4, 4}, {4, 4, 4}, {20, 10, 5}, {10, 5, 3}, {100, 45, 8}, {6, 7, 8}};
  std::vector<FloatVec3> lowerLeftCoords = {{-10, 5, 2},
                                            {0, 0, 0},
                                            {-0.997462, -0.997462, -0.00001},
                                            {-0.997462, -0.997462, -0.00001},
                                            {-0.997462, -0.997462, -0.00001},
                                            {-0.997462, -0.997462, -0.00001},
                                            {-0.997462, -0.997462, -0.00001},
                                            {0.9999989867210388, 0.9999989867210388, 1.5499989986419678}};
  std::vector<FloatVec3> upperRightCoords = {{15, 30, 27},
                                             {30, 30, 30},
                                             {0.997463, 0.997462, 0.991746},
                                             {0.997462, 0.997462, 0.991746},
                                             {0.997462, 0.997458, 0.991745},
                                             {0.997462, 0.997462, 0.991746},
                                             {0.997458, 0.99746, 0.99175},
                                             {7.630001068115234, 3.0000009536743164, 3.5500009059906006}};
  std::vector<std::string> amNames = {"CellData", "CellData", "VertexData", "VertexData", "VertexData", "VertexData", "VertexData", "VertexData"};
  std::vector<std::string> exemplaryArrayNames = {"ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds",
                                                  "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds", "ExemplaryPartitioningSchemeIds"};
  size_t lastIndex = 7;
  size_t index = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);

  SECTION("BasicPartitionArguments")
  {
    // Parallel parameter vectors must describe the same number of geometry scenarios.
    REQUIRE(filePaths.size() == lastIndex + 1);
    REQUIRE(partitionDimensions.size() == lastIndex + 1);
    REQUIRE(amNames.size() == lastIndex + 1);
    REQUIRE(exemplaryArrayNames.size() == lastIndex + 1);
    REQUIRE(lowerLeftCoords.size() == lastIndex + 1);
    REQUIRE(upperRightCoords.size() == lastIndex + 1);

    // The first geometry acquires the shared extracted fixture directory.
    if(index == 0)
    {
      s_FileSentinel = std::make_shared<FileSentinelType>(nx::core::unit_test::k_TestFilesDir, "PartitionGeometryTest.tar.gz", "PartitionGeometryTest");
    }

    std::cout << "Basic Partition Arguments: " << filePaths[index] << std::endl;
    const IntVec3 numOfPartitionsPerAxis = partitionDimensions[index];
    const DataPath inputGeometryPath = {{"DataContainer"}};
    DataPath attrMatrixPath = {{"DataContainer", amNames[index]}};

    DataStructure dataStructure;
    {
      const ReadDREAM3DFilter importD3DFilter;
      Arguments importD3DArgs;
      importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(filePaths[index]));
      // Execute the filter and check result
      auto executeResult = importD3DFilter.execute(dataStructure, importD3DArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }

    {
      Arguments partitionGeometryArgs =
          createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoords[index], upperRightCoords[index]);

      const PartitionGeometryFilter filter;
      // Execution must create partition identifiers for the selected geometry.
      auto executeResult = scope.executeFilter(filter, dataStructure, partitionGeometryArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      attrMatrixPath = partitionGeometryArgs.value<DataPath>(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key);
    }

    const Int32Array& partitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(partitionIdsArrayName));
    const Int32Array& exemplaryPartitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(exemplaryArrayNames[index]));

    REQUIRE(partitionIds.getSize() == exemplaryPartitionIds.getSize());

    const AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();
    const AbstractDataStore<int32>& exemplaryPartitionIdsStore = exemplaryPartitionIds.getDataStoreRef();
    for(size_t i = 0; i < partitionIds.getSize(); i++)
    {
      const int32_t partitionId = partitionIdsStore[i];
      const int32_t exemplaryId = exemplaryPartitionIdsStore[i];
      REQUIRE(partitionId == exemplaryId);
    }

    // The final geometry releases and removes the shared extracted fixtures.
    if(index == lastIndex)
    {
      s_FileSentinel = nullptr;
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Valid filter execution", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "PartitionGeometryTest.tar.gz", "PartitionGeometryTest");

  Arguments partitionGeometryArgs;
  Arguments importD3DArgs;

  const std::string partitionIdsArrayName = "PartitioningSchemeIds";
  const DataPath existingPSGeometryPath = {{"ExemplaryPSDataContainer"}};
  std::string exemplaryArrayName;

  SECTION("Test Existing Partitioning Scheme Image Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_ImageGeomTestFilePath));
  }

  SECTION("Test Existing Partitioning Scheme Rect Grid Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_RectGridGeomTestFilePath));
  }

  SECTION("Test Existing Partitioning Scheme Triangle Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_TriangleGeomTestFilePath));
  }

  SECTION("Test Existing Partitioning Scheme Edge Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_EdgeGeomTestFilePath));
  }

  SECTION("Test Existing Partitioning Scheme Vertex Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_VertexGeomTestFilePath));
  }

  SECTION("Test Existing Partitioning Scheme Quad Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_QuadGeomTestFilePath));
  }

  SECTION("Test Existing Partitioning Scheme Tetrahedral Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_TetrahedralGeomTestFilePath));
  }

  SECTION("Test Existing Partitioning Scheme Hexahedral Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_HexahedralGeomTestFilePath));
  }

  const ReadDREAM3DFilter importD3DFilter;
  DataStructure dataStructure;
  const PartitionGeometryFilter filter;

  // Preflight must accept the selected geometry and partition mode.
  auto executeResult = importD3DFilter.execute(dataStructure, importD3DArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Execution must create partition identifiers for the selected geometry.
  executeResult = scope.executeFilter(filter, dataStructure, partitionGeometryArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto attrMatrixPath = partitionGeometryArgs.value<DataPath>(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key);

  const Int32Array& partitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(partitionIdsArrayName));
  const Int32Array& exemplaryPartitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(exemplaryArrayName));

  REQUIRE(partitionIds.getSize() == exemplaryPartitionIds.getSize());

  const AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();
  const AbstractDataStore<int32>& exemplaryPartitionIdsStore = exemplaryPartitionIds.getDataStoreRef();
  for(size_t i = 0; i < partitionIds.getSize(); i++)
  {
    const int32_t partitionId = partitionIdsStore[i];
    const int32_t exemplaryId = exemplaryPartitionIdsStore[i];
    REQUIRE(partitionId == exemplaryId);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Invalid filter execution")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "PartitionGeometryTest.tar.gz", "PartitionGeometryTest");

  Arguments partitionGeometryArgs;
  Arguments importD3DArgs;

  const std::string partitionIdsArrayName = "PartitioningSchemeIds";
  const DataPath existingPSGeometryPath = {{"ExemplaryPSDataContainer"}};
  int expectedErrorCode = -1;

  SECTION("Test Planal XY Node Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {3, 3, 3};
    const DataPath inputGeometryPath = {{"VertexDataContainer"}};
    const DataPath attrMatrixPath = {{"VertexDataContainer", "AttributeMatrix"}};
    expectedErrorCode = -3042;

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_PlanalXYNodeGeomTestFilePath));
  }
  SECTION("Test Planal XZ Node Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {3, 3, 3};
    const DataPath inputGeometryPath = {{"VertexDataContainer"}};
    const DataPath attrMatrixPath = {{"VertexDataContainer", "AttributeMatrix"}};
    expectedErrorCode = -3041;

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_PlanalXZNodeGeomTestFilePath));
  }
  SECTION("Test Planal YZ Node Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {3, 3, 3};
    const DataPath inputGeometryPath = {{"VertexDataContainer"}};
    const DataPath attrMatrixPath = {{"VertexDataContainer", "AttributeMatrix"}};
    expectedErrorCode = -3040;

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ReadDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData(k_PlanalYZNodeGeomTestFilePath));
  }

  const ReadDREAM3DFilter importD3DFilter;
  DataStructure dataStructure;
  const PartitionGeometryFilter filter;

  auto executeResult = importD3DFilter.execute(dataStructure, importD3DArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  executeResult = filter.execute(dataStructure, partitionGeometryArgs);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == expectedErrorCode);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
