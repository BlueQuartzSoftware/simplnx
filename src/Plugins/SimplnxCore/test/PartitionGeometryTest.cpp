/**
 * @file PartitionGeometryTest.cpp
 * @brief Tests each partitioning mode, supported geometry, error path, and OOC algorithm path.
 */

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
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

  auto inputStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_BenchmarkInputArrayPath, cellTupleShape, {1});
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

const DataPath k_SelectedTupleGeometryPath({"Selected", "Geometries", "Geometry"});
const DataPath k_SelectedTupleMatrixPath({"Selected", "Arrays", "CellData"});
const DataPath k_SelectedVertexTupleMatrixPath({"Selected", "Arrays", "VertexData"});
const DataPath k_ExistingTuplePartitionGridPath({"ExistingTuplePartitionGrid"});

/**
 * @brief Creates an eight-cell grid and separately stored selected cell data.
 * @param dataStructure Receives the fixture and linked selected paths.
 * @param rectilinear Chooses a rectilinear grid instead of an image geometry.
 * @param tupleCount Number of tuples in the selected AttributeMatrix.
 */
void createCellTupleValidationInput(DataStructure& dataStructure, bool rectilinear, usize tupleCount)
{
  auto* storedGeometryGroupPtr = DataGroup::Create(dataStructure, "StoredGeometry");
  REQUIRE(storedGeometryGroupPtr != nullptr);
  IGridGeometry* geometryPtr = nullptr;
  if(rectilinear)
  {
    auto* rectGridPtr = RectGridGeom::Create(dataStructure, "Geometry", storedGeometryGroupPtr->getId());
    REQUIRE(rectGridPtr != nullptr);
    rectGridPtr->setDimensions({2, 2, 2});
    auto* xBoundsPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "XBounds", {3}, {1}, rectGridPtr->getId());
    auto* yBoundsPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "YBounds", {3}, {1}, rectGridPtr->getId());
    auto* zBoundsPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "ZBounds", {3}, {1}, rectGridPtr->getId());
    REQUIRE(xBoundsPtr != nullptr);
    REQUIRE(yBoundsPtr != nullptr);
    REQUIRE(zBoundsPtr != nullptr);
    for(usize index = 0; index < 3; index++)
    {
      (*xBoundsPtr)[index] = static_cast<float32>(index);
      (*yBoundsPtr)[index] = static_cast<float32>(index);
      (*zBoundsPtr)[index] = static_cast<float32>(index);
    }
    rectGridPtr->setXBoundsId(xBoundsPtr->getId());
    rectGridPtr->setYBoundsId(yBoundsPtr->getId());
    rectGridPtr->setZBoundsId(zBoundsPtr->getId());
    geometryPtr = rectGridPtr;
  }
  else
  {
    auto* imagePtr = ImageGeom::Create(dataStructure, "Geometry", storedGeometryGroupPtr->getId());
    REQUIRE(imagePtr != nullptr);
    imagePtr->setDimensions({2, 2, 2});
    imagePtr->setSpacing({1.0F, 1.0F, 1.0F});
    imagePtr->setOrigin({0.0F, 0.0F, 0.0F});
    geometryPtr = imagePtr;
  }
  auto* geometryCellDataPtr = AttributeMatrix::Create(dataStructure, "GeometryCellData", {2, 2, 2}, geometryPtr->getId());
  REQUIRE(geometryCellDataPtr != nullptr);
  geometryPtr->setCellData(*geometryCellDataPtr);
  auto* storedDataGroupPtr = DataGroup::Create(dataStructure, "StoredData");
  REQUIRE(storedDataGroupPtr != nullptr);
  // {4, 2} has eight tuples, with neither shape nor ownership matching the grid.
  const ShapeType tupleShape = tupleCount == 8 ? ShapeType{4, 2} : ShapeType{tupleCount};
  auto* selectedMatrixPtr = AttributeMatrix::Create(dataStructure, "CellData", tupleShape, storedDataGroupPtr->getId());
  REQUIRE(selectedMatrixPtr != nullptr);
  auto* selectedPtr = DataGroup::Create(dataStructure, "Selected");
  REQUIRE(selectedPtr != nullptr);
  auto* selectedGeometriesPtr = DataGroup::Create(dataStructure, "Geometries", selectedPtr->getId());
  auto* selectedArraysPtr = DataGroup::Create(dataStructure, "Arrays", selectedPtr->getId());
  REQUIRE(selectedGeometriesPtr != nullptr);
  REQUIRE(selectedArraysPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(geometryPtr->getId(), selectedGeometriesPtr->getId()));
  REQUIRE(dataStructure.setAdditionalParent(selectedMatrixPtr->getId(), selectedArraysPtr->getId()));
}

/**
 * @brief Configures one of the three modes that creates a partition grid.
 * @param mode Basic, advanced, or bounding-box mode index.
 * @return Arguments using the linked selected geometry and matrix paths.
 */
Arguments createCellTupleValidationArguments(ChoicesParameter::ValueType mode)
{
  Arguments args = createBasicPartitionGeometryArguments(k_SelectedTupleGeometryPath, k_SelectedTupleMatrixPath, "PartitionIds", {2, 2, 2}, {});
  args.insertOrAssign(PartitionGeometryFilter::k_PartitioningMode_Key, mode);
  args.insertOrAssign(PartitionGeometryFilter::k_CellLength_Key, std::vector<float32>{1.0F, 1.0F, 1.0F});
  args.insertOrAssign(PartitionGeometryFilter::k_MinGridCoord_Key, std::vector<float32>{0.0F, 0.0F, 0.0F});
  args.insertOrAssign(PartitionGeometryFilter::k_MaxGridCoord_Key, std::vector<float32>{2.0F, 2.0F, 2.0F});
  return args;
}

/**
 * @brief Creates eight cube corners and separately stored selected vertex data.
 * @param dataStructure Receives the geometry, linked selections, and an existing partition grid.
 * @param tupleCount Number of tuples in the selected AttributeMatrix.
 */
void createVertexTupleValidationInput(DataStructure& dataStructure, usize tupleCount)
{
  auto* storedGeometryGroupPtr = DataGroup::Create(dataStructure, "StoredGeometry");
  REQUIRE(storedGeometryGroupPtr != nullptr);
  auto* geometryPtr = VertexGeom::Create(dataStructure, "Geometry", storedGeometryGroupPtr->getId());
  REQUIRE(geometryPtr != nullptr);
  const DataPath coordinatesPath({"StoredGeometry", "Geometry", "Coordinates"});
  auto vertexStorePtr = DataStoreUtilities::CreateDataStore<float32>(dataStructure, coordinatesPath, {8}, {3});
  REQUIRE(vertexStorePtr != nullptr);
  auto* coordinatesPtr = Float32Array::Create(dataStructure, coordinatesPath.getTargetName(), vertexStorePtr, geometryPtr->getId());
  REQUIRE(coordinatesPtr != nullptr);
  constexpr std::array<float32, 24> cubeCorners = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F};
  const auto writeResult = vertexStorePtr->copyFromBuffer(0, nonstd::span<const float32>(cubeCorners.data(), cubeCorners.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  geometryPtr->setVertices(*coordinatesPtr);
  auto* geometryVertexDataPtr = AttributeMatrix::Create(dataStructure, "GeometryVertexData", {8}, geometryPtr->getId());
  REQUIRE(geometryVertexDataPtr != nullptr);
  geometryPtr->setVertexAttributeMatrix(*geometryVertexDataPtr);

  auto* storedDataGroupPtr = DataGroup::Create(dataStructure, "StoredData");
  REQUIRE(storedDataGroupPtr != nullptr);
  // The selected matrix can have a different tuple shape and parent than the vertex list.
  const ShapeType tupleShape = tupleCount == 8 ? ShapeType{4, 2} : ShapeType{tupleCount};
  auto* selectedMatrixPtr = AttributeMatrix::Create(dataStructure, "VertexData", tupleShape, storedDataGroupPtr->getId());
  REQUIRE(selectedMatrixPtr != nullptr);
  auto* selectedPtr = DataGroup::Create(dataStructure, "Selected");
  REQUIRE(selectedPtr != nullptr);
  auto* selectedGeometriesPtr = DataGroup::Create(dataStructure, "Geometries", selectedPtr->getId());
  auto* selectedArraysPtr = DataGroup::Create(dataStructure, "Arrays", selectedPtr->getId());
  REQUIRE(selectedGeometriesPtr != nullptr);
  REQUIRE(selectedArraysPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(geometryPtr->getId(), selectedGeometriesPtr->getId()));
  REQUIRE(dataStructure.setAdditionalParent(selectedMatrixPtr->getId(), selectedArraysPtr->getId()));
  REQUIRE(dataStructure.getDataAs<VertexGeom>(k_SelectedTupleGeometryPath) == geometryPtr);
  REQUIRE(dataStructure.getDataAs<AttributeMatrix>(k_SelectedVertexTupleMatrixPath) == selectedMatrixPtr);
  REQUIRE(geometryPtr->getNumberOfVertices() == 8);
  REQUIRE(selectedMatrixPtr->getNumberOfTuples() == tupleCount);

  auto* existingGridPtr = ImageGeom::Create(dataStructure, k_ExistingTuplePartitionGridPath.getTargetName());
  REQUIRE(existingGridPtr != nullptr);
  existingGridPtr->setDimensions({2, 2, 2});
  existingGridPtr->setOrigin({0.0F, 0.0F, 0.0F});
  existingGridPtr->setSpacing({1.0F, 1.0F, 1.0F});
}

/**
 * @brief Configures valid partition settings for the selected vertex fixture.
 * @param mode Basic, advanced, bounding-box, or existing-grid mode index.
 * @return Arguments using the linked selected geometry and vertex matrix paths.
 */
Arguments createVertexTupleValidationArguments(ChoicesParameter::ValueType mode)
{
  Arguments args = createCellTupleValidationArguments(mode);
  args.insertOrAssign(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, k_SelectedVertexTupleMatrixPath);
  args.insertOrAssign(PartitionGeometryFilter::k_ExistingPartitionGridPath_Key, k_ExistingTuplePartitionGridPath);
  args.insertOrAssign(PartitionGeometryFilter::k_PartitionGridGeometry_Key, DataPath({"CreatedTuplePartitionGrid"}));
  args.insertOrAssign(PartitionGeometryFilter::k_FeatureAttrMatrixName_Key, std::string("PartitionData"));
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Basic RectGrid bounds", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  std::vector<float32> xBounds;
  std::vector<float32> yBounds;
  std::vector<float32> zBounds;
  SECTION("Z spacing uses Z bounds with zero origins")
  {
    xBounds = {0, 1, 3, 6};
    yBounds = {0, 1, 2, 3, 5};
    zBounds = {0, 4, 10};
  }
  SECTION("Spacing uses extents with nonzero origins")
  {
    xBounds = {1, 2, 4, 7};
    yBounds = {-2, -1, 0, 1, 3};
    zBounds = {10, 14, 20};
  }

  DataStructure dataStructure;
  const DataPath geomPath({"RectGrid"});
  const DataPath cellDataPath = geomPath.createChildPath("Cell Data");
  const DataPath partitionGridPath({"Partition Grid"});
  auto* rectGeomPtr = RectGridGeom::Create(dataStructure, "RectGrid");
  REQUIRE(rectGeomPtr != nullptr);
  rectGeomPtr->setDimensions({3, 4, 2});
  auto* xArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "X Bounds", {xBounds.size()}, {1}, rectGeomPtr->getId());
  auto* yArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Y Bounds", {yBounds.size()}, {1}, rectGeomPtr->getId());
  auto* zArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Z Bounds", {zBounds.size()}, {1}, rectGeomPtr->getId());
  REQUIRE(xArrayPtr != nullptr);
  REQUIRE(yArrayPtr != nullptr);
  REQUIRE(zArrayPtr != nullptr);
  std::copy(xBounds.begin(), xBounds.end(), xArrayPtr->begin());
  std::copy(yBounds.begin(), yBounds.end(), yArrayPtr->begin());
  std::copy(zBounds.begin(), zBounds.end(), zArrayPtr->begin());
  const auto boundsResult = rectGeomPtr->setBounds(xArrayPtr, yArrayPtr, zArrayPtr);
  SIMPLNX_RESULT_REQUIRE_VALID(boundsResult);
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 4, 3}, rectGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  rectGeomPtr->setCellData(*cellDataPtr);

  auto args = createBasicPartitionGeometryArguments(geomPath, cellDataPath, "Partition Ids", {3, 5, 2}, std::nullopt);
  args.insertOrAssign(PartitionGeometryFilter::k_PartitionGridGeometry_Key, partitionGridPath);
  const PartitionGeometryFilter filter;
  const auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(partitionGridPath));
  const auto& partitionGeom = dataStructure.getDataRefAs<ImageGeom>(partitionGridPath);
  REQUIRE(partitionGeom.getDimensions() == SizeVec3{3, 5, 2});
  REQUIRE(partitionGeom.getOrigin() == FloatVec3{xBounds.front(), yBounds.front(), zBounds.front()});
  const auto spacing = partitionGeom.getSpacing();
  REQUIRE(spacing[0] == Approx(2.0f).margin(1.0e-5f));
  REQUIRE(spacing[1] == Approx(1.0f).margin(1.0e-5f));
  REQUIRE(spacing[2] == Approx(5.0f).margin(1.0e-5f));

  const auto partitionIdsPath = cellDataPath.createChildPath("Partition Ids");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(partitionIdsPath));
  const auto& partitionIds = dataStructure.getDataRefAs<Int32Array>(partitionIdsPath);
  // Cell centers map to partition coordinates (0,0,0), (1,1,0), and (2,4,1).
  // With starting ID 1, the partition ID is 1 + x + 3*y + 15*z.
  REQUIRE(partitionIds[0] == 1);
  REQUIRE(partitionIds[4] == 5);
  REQUIRE(partitionIds[23] == 30);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Selected cell tuple counts", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const bool rectilinear = GENERATE(false, true);
  const auto mode = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1}, ChoicesParameter::ValueType{2});
  const usize tupleCount = GENERATE(usize{7}, usize{8}, usize{9});
  CAPTURE(rectilinear, mode, tupleCount);
  DataStructure dataStructure;
  createCellTupleValidationInput(dataStructure, rectilinear, tupleCount);
  const PartitionGeometryFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, createCellTupleValidationArguments(mode));
  if(tupleCount == 8)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    const auto& error = preflightResult.outputActions.errors()[0];
    CHECK(error.code == (rectilinear ? -3010 : -3009));
    CHECK(error.message.find(k_SelectedTupleGeometryPath.toString()) != std::string::npos);
    CHECK(error.message.find(k_SelectedTupleMatrixPath.toString()) != std::string::npos);
    CHECK(error.message.find(fmt::format("has {} tuples", tupleCount)) != std::string::npos);
    CHECK(error.message.find("has 8 cells") != std::string::npos);
    CHECK(error.message.find("one tuple per geometry cell") != std::string::npos);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Partition counts precede image tuple validation", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const auto mode = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1}, ChoicesParameter::ValueType{2});
  const usize axis = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(mode, axis);
  DataStructure dataStructure;
  createCellTupleValidationInput(dataStructure, false, 7);
  Arguments args = createCellTupleValidationArguments(mode);
  std::vector<int32> partitionCounts = {2, 2, 2};
  partitionCounts[axis] = 0;
  args.insertOrAssign(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, partitionCounts);
  // A partition-count error also precedes invalid advanced lengths/bounds.
  args.insertOrAssign(PartitionGeometryFilter::k_CellLength_Key, std::vector<float32>{-1.0F, -1.0F, -1.0F});
  args.insertOrAssign(PartitionGeometryFilter::k_MinGridCoord_Key, std::vector<float32>{3.0F, 3.0F, 3.0F});
  const auto preflightResult = PartitionGeometryFilter().preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  CHECK(preflightResult.outputActions.errors()[0].code == -3012 - static_cast<int32>(axis));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Selected vertex tuple counts", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const auto mode = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1}, ChoicesParameter::ValueType{2}, ChoicesParameter::ValueType{3});
  const usize tupleCount = GENERATE(usize{7}, usize{8}, usize{9});
  CAPTURE(mode, tupleCount);
  DataStructure dataStructure;
  createVertexTupleValidationInput(dataStructure, tupleCount);
  const auto initialSize = dataStructure.getSize();
  const PartitionGeometryFilter filter;
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = filter.preflight(dataStructure, createVertexTupleValidationArguments(mode)));
  if(tupleCount == 8)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    const auto& error = preflightResult.outputActions.errors()[0];
    CHECK(error.code == -3043);
    CHECK(error.code != -3014);
    CHECK(error.message.find(k_SelectedTupleGeometryPath.toString()) != std::string::npos);
    CHECK(error.message.find(k_SelectedVertexTupleMatrixPath.toString()) != std::string::npos);
    CHECK(error.message.find(fmt::format("has {} tuples", tupleCount)) != std::string::npos);
    CHECK(error.message.find("has 8 vertices") != std::string::npos);
    CHECK(error.message.find("one tuple per geometry vertex") != std::string::npos);
    CHECK(error.message.find(filter.humanName() + ":") == std::string::npos);
  }
  CHECK(dataStructure.getSize() == initialSize);
  CHECK_FALSE(dataStructure.containsData(k_SelectedVertexTupleMatrixPath.createChildPath("PartitionIds")));
  CHECK_FALSE(dataStructure.containsData(k_SelectedVertexTupleMatrixPath.getParent().createChildPath("PartitionData")));
  CHECK_FALSE(dataStructure.containsData(DataPath({"CreatedTuplePartitionGrid"})));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Rejects unsupported active vertex mask kinds", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const auto mode = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1}, ChoicesParameter::ValueType{2}, ChoicesParameter::ValueType{3});
  const bool useStringArray = GENERATE(false, true);
  CAPTURE(mode, useStringArray);
  DataStructure dataStructure;
  createVertexTupleValidationInput(dataStructure, 8);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_SelectedVertexTupleMatrixPath));
  const auto& parent = dataStructure.getDataRefAs<AttributeMatrix>(k_SelectedVertexTupleMatrixPath);
  const DataPath maskPath = k_SelectedVertexTupleMatrixPath.createChildPath("RejectedMask");
  if(useStringArray)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, maskPath.getTargetName(), parent.getShape(), std::vector<std::string>(8, "true"), parent.getId()) != nullptr);
  }
  else
  {
    auto* listsPtr = NeighborList<int32>::Create(dataStructure, maskPath.getTargetName(), parent.getShape(), parent.getId());
    REQUIRE(listsPtr != nullptr);
    for(usize index = 0; index < parent.getNumberOfTuples(); index++)
    {
      listsPtr->setList(static_cast<int32>(index), std::vector<int32>{1});
    }
    REQUIRE(listsPtr->getTupleShape() == parent.getShape());
  }
  auto args = createVertexTupleValidationArguments(mode);
  args.insertOrAssign(PartitionGeometryFilter::k_UseVertexMask_Key, true);
  args.insertOrAssign(PartitionGeometryFilter::k_VertexMaskPath_Key, maskPath);
  const auto initialPaths = dataStructure.getAllDataPaths();
  const auto initialNextId = dataStructure.getNextId();
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = PartitionGeometryFilter().preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  const auto& error = preflightResult.outputActions.errors().front();
  CHECK(error.code == -3044);
  CHECK(error.message.find(maskPath.toString()) != std::string::npos);
  CHECK(error.message.find(useStringArray ? "StringArray" : "NeighborList<int32>") != std::string::npos);
  CHECK(error.message.find("scalar Boolean DataArray") != std::string::npos);
  CHECK(error.message.find("Select a scalar Boolean DataArray") != std::string::npos);
  CHECK(dataStructure.getAllDataPaths() == initialPaths);
  CHECK(dataStructure.getNextId() == initialNextId);
  CHECK_FALSE(dataStructure.containsData(k_SelectedVertexTupleMatrixPath.createChildPath("PartitionIds")));
  CHECK_FALSE(dataStructure.containsData(k_SelectedVertexTupleMatrixPath.getParent().createChildPath("PartitionData")));
  CHECK_FALSE(dataStructure.containsData(DataPath({"CreatedTuplePartitionGrid"})));
  if(useStringArray)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(maskPath));
    CHECK(dataStructure.getDataRefAs<StringArray>(maskPath)[0] == "true");
  }
  else
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(maskPath));
    CHECK(dataStructure.getDataRefAs<NeighborList<int32>>(maskPath).getList(0) == std::vector<int32>{1});
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Inactive mask selections and active Boolean arrays pass preflight", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const usize maskKind = GENERATE(usize{0}, usize{1}, usize{2}, usize{3}, usize{4});
  CAPTURE(maskKind);
  DataStructure dataStructure;
  createVertexTupleValidationInput(dataStructure, 8);
  const DataPath maskPath({"OptionalMask"});
  if(maskKind == 0)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, maskPath.getTargetName(), {1}, {"unused"}) != nullptr);
  }
  else if(maskKind == 1)
  {
    REQUIRE(NeighborList<int32>::Create(dataStructure, maskPath.getTargetName(), {1}) != nullptr);
  }
  else if(maskKind >= 3)
  {
    auto maskStorePtr = DataStoreUtilities::CreateDataStore<bool>(dataStructure, maskPath, {8}, {1});
    auto* maskPtr = BoolArray::Create(dataStructure, maskPath.getTargetName(), maskStorePtr);
    REQUIRE(maskPtr != nullptr);
    maskPtr->fill(true);
  }
  auto args = createVertexTupleValidationArguments(0);
  args.insertOrAssign(PartitionGeometryFilter::k_UseVertexMask_Key, maskKind == 4);
  args.insertOrAssign(PartitionGeometryFilter::k_VertexMaskPath_Key, maskPath);
  const auto initialPaths = dataStructure.getAllDataPaths();
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = PartitionGeometryFilter().preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  CHECK(dataStructure.getAllDataPaths() == initialPaths);
  CHECK_FALSE(dataStructure.containsData(k_SelectedVertexTupleMatrixPath.createChildPath("PartitionIds")));
  CHECK_FALSE(dataStructure.containsData(DataPath({"CreatedTuplePartitionGrid"})));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Geometry errors precede vertex mask kind errors", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const usize geometryKind = GENERATE(usize{0}, usize{1}, usize{2});
  const bool tupleMismatch = GENERATE(false, true);
  const bool useStringArray = GENERATE(false, true);
  CAPTURE(geometryKind, tupleMismatch, useStringArray);
  DataStructure dataStructure;
  const usize tupleCount = tupleMismatch ? 7 : 8;
  Arguments args;
  if(geometryKind == 0)
  {
    createVertexTupleValidationInput(dataStructure, tupleCount);
    args = createVertexTupleValidationArguments(0);
  }
  else
  {
    createCellTupleValidationInput(dataStructure, geometryKind == 2, tupleCount);
    args = createCellTupleValidationArguments(0);
  }
  const DataPath maskPath({"RejectedMask"});
  if(useStringArray)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, maskPath.getTargetName(), {8}, std::vector<std::string>(8, "true")) != nullptr);
  }
  else
  {
    REQUIRE(NeighborList<int32>::Create(dataStructure, maskPath.getTargetName(), {8}) != nullptr);
  }
  args.insertOrAssign(PartitionGeometryFilter::k_UseVertexMask_Key, true);
  args.insertOrAssign(PartitionGeometryFilter::k_VertexMaskPath_Key, maskPath);
  const auto initialPaths = dataStructure.getAllDataPaths();
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = PartitionGeometryFilter().preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  constexpr std::array<int32, 3> tupleErrorCodes = {-3043, -3009, -3010};
  int32 expectedCode = -3019;
  if(tupleMismatch)
  {
    expectedCode = tupleErrorCodes.at(geometryKind);
  }
  else if(geometryKind == 0)
  {
    expectedCode = -3044;
  }
  CHECK(preflightResult.outputActions.errors().front().code == expectedCode);
  CHECK(dataStructure.getAllDataPaths() == initialPaths);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Vertex tuple and partition Z count errors are distinct", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure vertexDataStructure;
  createVertexTupleValidationInput(vertexDataStructure, 7);
  const auto vertexPreflight = PartitionGeometryFilter().preflight(vertexDataStructure, createVertexTupleValidationArguments(0));
  SIMPLNX_RESULT_REQUIRE_INVALID(vertexPreflight.outputActions);
  REQUIRE(vertexPreflight.outputActions.errors().size() == 1);
  CHECK(vertexPreflight.outputActions.errors()[0].code == -3043);

  DataStructure imageDataStructure;
  createCellTupleValidationInput(imageDataStructure, false, 8);
  Arguments imageArgs = createCellTupleValidationArguments(0);
  imageArgs.insertOrAssign(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, std::vector<int32>{2, 2, 0});
  const auto imagePreflight = PartitionGeometryFilter().preflight(imageDataStructure, imageArgs);
  SIMPLNX_RESULT_REQUIRE_INVALID(imagePreflight.outputActions);
  REQUIRE(imagePreflight.outputActions.errors().size() == 1);
  CHECK(imagePreflight.outputActions.errors()[0].code == -3014);
  CHECK(imagePreflight.outputActions.errors()[0].message.find("Z dimension (0)") != std::string::npos);
  CHECK(vertexPreflight.outputActions.errors()[0].code != imagePreflight.outputActions.errors()[0].code);
  UnitTest::CheckArraysInheritTupleDims(vertexDataStructure);
  UnitTest::CheckArraysInheritTupleDims(imageDataStructure);
}

TEST_CASE("SimplnxCore::PartitionGeometryFilter: Mode errors precede image tuple validation", "[Plugins][PartitionGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const auto mode = GENERATE(ChoicesParameter::ValueType{1}, ChoicesParameter::ValueType{2});
  const usize axis = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(mode, axis);
  DataStructure dataStructure;
  createCellTupleValidationInput(dataStructure, false, 7);
  Arguments args = createCellTupleValidationArguments(mode);
  std::vector<float32> invalidValues = {1.0F, 1.0F, 1.0F};
  if(mode == static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Advanced))
  {
    invalidValues[axis] = -1.0F;
    args.insertOrAssign(PartitionGeometryFilter::k_CellLength_Key, invalidValues);
  }
  else
  {
    invalidValues[axis] = 3.0F;
    args.insertOrAssign(PartitionGeometryFilter::k_MinGridCoord_Key, invalidValues);
  }
  const auto preflightResult = PartitionGeometryFilter().preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  const int32 firstAxisError = mode == static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Advanced) ? -3003 : -3006;
  CHECK(preflightResult.outputActions.errors()[0].code == firstAxisError - static_cast<int32>(axis));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

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
