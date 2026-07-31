#include "SimplnxCore/Filters/ComputeFeatureNeighborsFilter.hpp"
#include "SimplnxCore/Filters/RequireMinNumNeighborsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
namespace DiscriminatingFixture
{
constexpr usize k_Dimension = 6;
constexpr usize k_CellCount = k_Dimension * k_Dimension * k_Dimension;
constexpr usize k_FeatureCount = 6;

struct Coordinate
{
  usize x = 0;
  usize y = 0;
  usize z = 0;

  constexpr bool isAt(usize pointX, usize pointY, usize pointZ) const
  {
    return x == pointX && y == pointY && z == pointZ;
  }
};

constexpr Coordinate k_TieCell = {5, 5, 5};
constexpr Coordinate k_TieNegativeXCell = {4, 5, 5};
constexpr Coordinate k_PositiveXBoundaryCell = {0, 5, 5};
constexpr Coordinate k_PositiveYBoundaryCell = {5, 0, 5};
constexpr Coordinate k_Feature3Seed = {5, 5, 4};
constexpr Coordinate k_Feature4Seed = {5, 4, 5};
constexpr std::array<float32, 3> k_VectorOffsets = {0.25F, 0.5F, 0.75F};

constexpr usize GetIndex(usize x, usize y, usize z)
{
  return z * k_Dimension * k_Dimension + y * k_Dimension + x;
}

constexpr usize GetIndex(const Coordinate& coordinate)
{
  return GetIndex(coordinate.x, coordinate.y, coordinate.z);
}

constexpr bool IsRejectedCubeCell(usize x, usize y, usize z)
{
  return x >= 1 && x <= 3 && y >= 1 && y <= 3 && z >= 1 && z <= 3;
}

constexpr bool IsRejectedCell(usize x, usize y, usize z)
{
  return IsRejectedCubeCell(x, y, z) || k_TieCell.isAt(x, y, z) || k_TieNegativeXCell.isAt(x, y, z) || k_PositiveXBoundaryCell.isAt(x, y, z) || k_PositiveYBoundaryCell.isAt(x, y, z);
}

constexpr int32 GetInputFeatureId(usize x, usize y, usize z)
{
  if(IsRejectedCell(x, y, z))
  {
    return 1;
  }
  if(k_Feature3Seed.isAt(x, y, z))
  {
    return 3;
  }
  if(k_Feature4Seed.isAt(x, y, z))
  {
    return 4;
  }
  return 2;
}

constexpr usize GetExpectedSourceIndex(usize x, usize y, usize z)
{
  if(k_TieCell.isAt(x, y, z))
  {
    return GetIndex(k_Feature3Seed);
  }
  if(k_TieNegativeXCell.isAt(x, y, z))
  {
    return GetIndex(3, 5, 5);
  }
  if(k_PositiveXBoundaryCell.isAt(x, y, z))
  {
    return GetIndex(1, 5, 5);
  }
  if(k_PositiveYBoundaryCell.isAt(x, y, z))
  {
    return GetIndex(5, 1, 5);
  }
  if(!IsRejectedCubeCell(x, y, z))
  {
    return GetIndex(x, y, z);
  }

  // The cube center is filled during the second coarsening pass. Its final +Z
  // neighbor copied its tuple from the exterior cell at z = 4 during pass one.
  if(x == 2 && y == 2 && z == 2)
  {
    return GetIndex(2, 2, 4);
  }

  // Neighbor traversal is -Z, -Y, -X, +X, +Y, +Z. When every valid exterior
  // neighbor has feature 2, the last matching vote supplies the copied tuple.
  if(z == 3)
  {
    return GetIndex(x, y, 4);
  }
  if(y == 3)
  {
    return GetIndex(x, 4, z);
  }
  if(x == 3)
  {
    return GetIndex(4, y, z);
  }
  if(x == 1)
  {
    return GetIndex(0, y, z);
  }
  if(y == 1)
  {
    return GetIndex(x, 0, z);
  }
  return GetIndex(x, y, 0);
}

constexpr int32 GetExpectedFeatureId(usize x, usize y, usize z)
{
  if(k_TieCell.isAt(x, y, z) || k_Feature3Seed.isAt(x, y, z))
  {
    return 2;
  }
  if(k_Feature4Seed.isAt(x, y, z))
  {
    return 3;
  }
  return 1;
}

void PopulateDataStructure(DataStructure& dataStructure)
{
  const SizeVec3 imageSize = {k_Dimension, k_Dimension, k_Dimension};
  const ShapeType cellShape = {k_Dimension, k_Dimension, k_Dimension};

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {k_FeatureCount}, imageGeom->getId());

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsStore, cellAM->getId());
  auto copiedScalarStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* copiedScalar = DataArray<int32>::Create(dataStructure, "CopiedScalar", copiedScalarStore, cellAM->getId());
  auto copiedVectorStore = DataStoreUtilities::CreateDataStore<float32>(cellShape, {3}, IDataAction::Mode::Execute);
  auto* copiedVector = DataArray<float32>::Create(dataStructure, "CopiedVector", copiedVectorStore, cellAM->getId());
  auto ignoredValuesStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* ignoredValues = DataArray<int32>::Create(dataStructure, "IgnoredValues", ignoredValuesStore, cellAM->getId());
  auto numNeighborsStore = DataStoreUtilities::CreateDataStore<int32>({k_FeatureCount}, {1}, IDataAction::Mode::Execute);
  auto* numNeighbors = DataArray<int32>::Create(dataStructure, "NumNeighbors", numNeighborsStore, featureAM->getId());
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>({k_FeatureCount}, {1}, IDataAction::Mode::Execute);
  auto* phases = DataArray<int32>::Create(dataStructure, "Phases", phasesStore, featureAM->getId());

  for(usize z = 0; z < k_Dimension; z++)
  {
    for(usize y = 0; y < k_Dimension; y++)
    {
      for(usize x = 0; x < k_Dimension; x++)
      {
        const usize index = GetIndex(x, y, z);
        featureIds->getDataStoreRef()[index] = GetInputFeatureId(x, y, z);
        copiedScalar->getDataStoreRef()[index] = static_cast<int32>(10000 + index);
        ignoredValues->getDataStoreRef()[index] = static_cast<int32>(20000 + index);
        for(usize component = 0; component < k_VectorOffsets.size(); component++)
        {
          copiedVector->getDataStoreRef()[index * k_VectorOffsets.size() + component] = static_cast<float32>(index) + k_VectorOffsets[component];
        }
      }
    }
  }

  const std::array<int32, k_FeatureCount> inputNumNeighbors = {0, 0, 3, 4, 5, 0};
  const std::array<int32, k_FeatureCount> inputPhases = {0, 1, 1, 1, 1, 2};
  for(usize featureId = 0; featureId < k_FeatureCount; featureId++)
  {
    numNeighbors->getDataStoreRef()[featureId] = inputNumNeighbors[featureId];
    phases->getDataStoreRef()[featureId] = inputPhases[featureId];
  }
}
} // namespace DiscriminatingFixture
} // namespace

// Oracle strategy and expected-value calculation:
//
// 1. The 4x1x1 oracle starts with FeatureIds [2, 1, 1, 3]. Feature 1 is
//    rejected. Its left cell has feature 2 as its only valid retained
//    neighbor, and its right cell has feature 3. Their copied scalar values
//    therefore come from those neighbors, producing [20, 20, 30, 30].
//    Removing feature 1 then compacts feature 2 to ID 1 and feature 3 to ID 2,
//    producing FeatureIds [1, 1, 2, 2].
//
// 2. The 6x6x6 oracle rejects the 27 cells in a 3x3x3 feature-1 cube plus
//    four additional feature-1 cells, giving 31 initially rejected cells.
//    The cube surface and additional cells have retained face neighbors and
//    are filled during the first pass. Only the cube center remains, so the
//    observed coarsening counts are 31, 1, and 0. The center is filled during
//    the second pass from its final +Z neighbor, whose tuple was copied from
//    the original exterior cell at (2, 2, 4).
//
//    The tie cell has feature 3 at -Z and feature 4 at -Y. Face neighbors are
//    visited in the order -Z, -Y, -X, +X, +Y, +Z. Because the selected source
//    changes only when a vote count is strictly greater than the current
//    maximum, the 1-vs-1 tie retains the first vote and selects feature 3.
//    For equal-feature exterior neighbors, each later vote increases that
//    feature's count, so the last valid matching neighbor supplies the tuple.
//    The expected source-index function encodes this traversal order directly.
//
//    Feature compaction maps retained input features 2, 3, and 4 to output
//    IDs 1, 2, and 3. Feature 5 owns no cells. It is removed in all-phase mode
//    but retained in single-phase mode because it belongs to another phase.
//    This gives feature-array lengths of 4 and 5 while all cell arrays remain
//    identical. Scalar, float32[3], ignored-array, NumNeighbors, and Phases
//    expectations are calculated directly from these source indices and
//    compaction mappings.

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle", "[SimplnxCore][RequireMinNumNeighborsFilter]")
{
  UnitTest::LoadPlugins();

  const bool singlePhase = GENERATE(false, true);
  DYNAMIC_SECTION("ApplyToSinglePhase=" << singlePhase)
  {
    // Class 1 oracle: feature 1 occupies the two middle cells of [2, 1, 1, 3]
    // and is rejected. Its left/right face-neighbor votes select features 2 and 3,
    // yielding FeatureIds [1, 1, 2, 2] after inactive-feature removal remaps IDs.
    DataStructure dataStructure;

    const SizeVec3 imageSize = {4, 1, 1};
    const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

    auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeom->setDimensions(imageSize);
    auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

    auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
    auto* copiedValues = UnitTest::CreateTestDataArray<int32>(dataStructure, "CopiedValues", arraySize, {1}, cellAM->getId());
    auto* ignoredValues = UnitTest::CreateTestDataArray<int32>(dataStructure, "IgnoredValues", arraySize, {1}, cellAM->getId());
    auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
    auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {4}, {1}, featureAM->getId());

    const std::array<int32, 4> inputFeatureIds = {2, 1, 1, 3};
    const std::array<int32, 4> inputCopiedValues = {20, 101, 102, 30};
    const std::array<int32, 4> inputIgnoredValues = {200, 101, 102, 300};
    const std::array<int32, 4> inputNumNeighbors = singlePhase ? std::array<int32, 4>{0, 0, 0, 3} : std::array<int32, 4>{0, 0, 3, 3};
    const std::array<int32, 4> inputPhases = {0, 1, 2, 1};
    for(usize i = 0; i < inputFeatureIds.size(); i++)
    {
      featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
      copiedValues->getDataStoreRef()[i] = inputCopiedValues[i];
      ignoredValues->getDataStoreRef()[i] = inputIgnoredValues[i];
      numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
      phases->getDataStoreRef()[i] = inputPhases[i];
    }

    RequireMinNumNeighborsFilter filter;
    Arguments args;
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(singlePhase));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(1));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "Phases"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({"ImageGeometry", "CellData", "IgnoredValues"})}));

    SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
    SIMPLNX_RESULT_REQUIRE_VALID(filter.execute(dataStructure, args).result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
    const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"}));
    const auto& outputCopiedValues = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "CopiedValues"}));
    const auto& outputIgnoredValues = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "IgnoredValues"}));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "Phases"})));
    const auto& outputNumNeighbors = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"}));
    const auto& outputPhases = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "Phases"}));
    const std::array<int32, 4> expectedFeatureIds = {1, 1, 2, 2};
    const std::array<int32, 4> expectedCopiedValues = {20, 20, 30, 30};
    const std::array<int32, 3> expectedNumNeighbors = singlePhase ? std::array<int32, 3>{0, 0, 3} : std::array<int32, 3>{0, 3, 3};
    const std::array<int32, 3> expectedPhases = {0, 2, 1};
    for(usize i = 0; i < expectedFeatureIds.size(); i++)
    {
      CHECK(outputFeatureIds[i] == expectedFeatureIds[i]);
      CHECK(outputCopiedValues[i] == expectedCopiedValues[i]);
      CHECK(outputIgnoredValues[i] == inputIgnoredValues[i]);
      // Class 4 invariants: reassignment leaves no rejected IDs and IDs remain valid.
      CHECK(outputFeatureIds[i] >= 0);
      CHECK(outputFeatureIds[i] < 3);
    }
    REQUIRE(outputNumNeighbors.getNumberOfTuples() == expectedNumNeighbors.size());
    REQUIRE(outputPhases.getNumberOfTuples() == expectedPhases.size());
    for(usize i = 0; i < expectedNumNeighbors.size(); i++)
    {
      CHECK(outputNumNeighbors[i] == expectedNumNeighbors[i]);
      CHECK(outputPhases[i] == expectedPhases[i]);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Discriminating 6x6x6 Analytical Fixture", "[SimplnxCore][RequireMinNumNeighborsFilter]")
{
  UnitTest::LoadPlugins();

  std::vector<int32> allPhaseFeatureIds;
  for(const bool singlePhase : std::array<bool, 2>{false, true})
  {
    INFO("ApplyToSinglePhase=" << singlePhase);

    DataStructure dataStructure;
    DiscriminatingFixture::PopulateDataStructure(dataStructure);

    const DataPath featureIdsPath({"ImageGeometry", "CellData", "FeatureIds"});
    const DataPath copiedScalarPath({"ImageGeometry", "CellData", "CopiedScalar"});
    const DataPath copiedVectorPath({"ImageGeometry", "CellData", "CopiedVector"});
    const DataPath ignoredValuesPath({"ImageGeometry", "CellData", "IgnoredValues"});
    const DataPath numNeighborsPath({"ImageGeometry", "FeatureData", "NumNeighbors"});
    const DataPath phasesPath({"ImageGeometry", "FeatureData", "Phases"});

    RequireMinNumNeighborsFilter filter;
    Arguments args;
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(3));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(singlePhase));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(1));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(phasesPath));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(numNeighborsPath));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{ignoredValuesPath}));

    SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);

    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featureIdsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(copiedScalarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(copiedVectorPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(ignoredValuesPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(numNeighborsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(phasesPath));
    const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
    const auto& outputCopiedScalar = dataStructure.getDataRefAs<Int32Array>(copiedScalarPath);
    const auto& outputCopiedVector = dataStructure.getDataRefAs<Float32Array>(copiedVectorPath);
    const auto& outputIgnoredValues = dataStructure.getDataRefAs<Int32Array>(ignoredValuesPath);
    const auto& outputNumNeighbors = dataStructure.getDataRefAs<Int32Array>(numNeighborsPath);
    const auto& outputPhases = dataStructure.getDataRefAs<Int32Array>(phasesPath);
    REQUIRE(outputFeatureIds.getNumberOfTuples() == DiscriminatingFixture::k_CellCount);

    for(usize z = 0; z < DiscriminatingFixture::k_Dimension; z++)
    {
      for(usize y = 0; y < DiscriminatingFixture::k_Dimension; y++)
      {
        for(usize x = 0; x < DiscriminatingFixture::k_Dimension; x++)
        {
          const usize index = DiscriminatingFixture::GetIndex(x, y, z);
          const usize sourceIndex = DiscriminatingFixture::GetExpectedSourceIndex(x, y, z);
          CAPTURE(x, y, z, index, sourceIndex, singlePhase);
          CHECK(outputFeatureIds[index] == DiscriminatingFixture::GetExpectedFeatureId(x, y, z));
          CHECK(outputCopiedScalar[index] == static_cast<int32>(10000 + sourceIndex));
          CHECK(outputIgnoredValues[index] == static_cast<int32>(20000 + index));
          for(usize component = 0; component < DiscriminatingFixture::k_VectorOffsets.size(); component++)
          {
            CHECK(outputCopiedVector[index * DiscriminatingFixture::k_VectorOffsets.size() + component] == static_cast<float32>(sourceIndex) + DiscriminatingFixture::k_VectorOffsets[component]);
          }
        }
      }
    }

    const std::vector<int32> expectedNumNeighbors = singlePhase ? std::vector<int32>{0, 3, 4, 5, 0} : std::vector<int32>{0, 3, 4, 5};
    const std::vector<int32> expectedPhases = singlePhase ? std::vector<int32>{0, 1, 1, 1, 2} : std::vector<int32>{0, 1, 1, 1};
    REQUIRE(outputNumNeighbors.getNumberOfTuples() == expectedNumNeighbors.size());
    REQUIRE(outputPhases.getNumberOfTuples() == expectedPhases.size());
    for(usize featureId = 0; featureId < expectedNumNeighbors.size(); featureId++)
    {
      CHECK(outputNumNeighbors[featureId] == expectedNumNeighbors[featureId]);
      CHECK(outputPhases[featureId] == expectedPhases[featureId]);
    }

    std::vector<int32> currentFeatureIds(outputFeatureIds.getNumberOfTuples());
    for(usize index = 0; index < outputFeatureIds.getNumberOfTuples(); index++)
    {
      currentFeatureIds[index] = outputFeatureIds[index];
    }
    if(singlePhase)
    {
      REQUIRE(currentFeatureIds == allPhaseFeatureIds);
    }
    else
    {
      allPhaseFeatureIds = currentFeatureIds;
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter", "[SimplnxCore][RequireMinNumNeighborsFilter]")
{
  UnitTest::LoadPlugins();

  constexpr int32 k_MinNumNeighbors = 3;
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(k_CellData);
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
  std::string numNeighborName = "NumNeighbors2";
  std::string neighborListName = "NeighborList2";
  std::string sharedSurfaceAreaListName = "SharedSurfaceAreaList2";
  std::string boundaryCellsName = "BoundaryCells_computed";
  std::string surfaceFeaturesName = "SurfaceFeatures_computed";
  DataPath numNeighborPath = cellFeatureAttributeMatrixPath.createChildPath(numNeighborName);
  DataPath neighborListPath = cellFeatureAttributeMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaListPath = cellFeatureAttributeMatrixPath.createChildPath(sharedSurfaceAreaListName);
  DataPath boundaryCellsPath = cellFeatureAttributeMatrixPath.createChildPath(boundaryCellsName);
  DataPath surfaceFeaturesPath = cellFeatureAttributeMatrixPath.createChildPath(surfaceFeaturesName);

  {
    ComputeFeatureNeighborsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_CellFeaturesPath_Key, std::make_any<DataPath>(cellFeatureAttributeMatrixPath));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_BoundaryCellsName_Key, std::make_any<std::string>(boundaryCellsName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreSurface_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SurfaceFeaturesName_Key, std::make_any<std::string>(surfaceFeaturesName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NumNeighborsName_Key, std::make_any<std::string>(numNeighborName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NeighborListName_Key, std::make_any<std::string>(neighborListName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SharedSurfaceAreaName_Key, std::make_any<std::string>(sharedSurfaceAreaListName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  DataPath numElementsPath = cellFeatureAttributeMatrixPath.createChildPath("NumElements");
  const auto& inputNumElements = dataStructure.getDataRefAs<Int32Array>(numElementsPath);
  const auto& computedNumNeighbors = dataStructure.getDataRefAs<Int32Array>(numNeighborPath);
  REQUIRE(inputNumElements.getNumberOfTuples() == computedNumNeighbors.getNumberOfTuples());

  // Feature 0 is retained. Other feature tuples meeting the minimum-neighbor
  // threshold are retained in their original order.
  std::vector<int32> expectedNumElements;
  expectedNumElements.reserve(inputNumElements.getNumberOfTuples());
  for(usize featureId = 0; featureId < inputNumElements.getNumberOfTuples(); featureId++)
  {
    if(featureId == 0 || computedNumNeighbors[featureId] >= k_MinNumNeighbors)
    {
      expectedNumElements.push_back(inputNumElements[featureId]);
    }
  }

  {
    RequireMinNumNeighborsFilter filter;
    Arguments args;

    args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(k_MinNumNeighbors));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(numNeighborPath));
    // args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    CHECK(preflightResult.outputActions.warnings()[0].code == -5558);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const Int32Array& createdFeatureArray = dataStructure.getDataRefAs<Int32Array>(numElementsPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == expectedNumElements.size());

    for(usize i = 0; i < expectedNumElements.size(); i++)
    {
      REQUIRE(expectedNumElements[i] == createdFeatureArray[i]);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/minimum_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // With "Apply to Single Phase Only" enabled, preflight validates that the feature-level
  // NumNeighbors and FeaturePhases arrays share the same tuple count. Build them with
  // deliberately different tuple counts (in separate AttributeMatrices) to drive the
  // validateNumberOfTuples() guard that emits error -252.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({4, 1, 1});

  // Cell Data: FeatureIds is dereferenced in preflight (must exist) but is not part of the
  // feature-level tuple-count check.
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {4}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {4}, {1}, cellAM->getId());

  // Feature Data: NumNeighbors has 5 tuples.
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {5}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {5}, {1}, featureAM->getId());

  // FeaturePhases lives in a separate AttributeMatrix with a different tuple count (4 != 5),
  // causing the cross-array tuple-count check to fail.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {4}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {4}, {1}, mismatchAM->getId());

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -252);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds tuple count mismatch (-55571)", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({5, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 4}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 1, 4}, {1}, cellAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -55571);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - feature ID out of range (-55567)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {1, 4, 2, 3};
  const std::array<int32, 4> inputNumNeighbors = {0, 0, 3, 3};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -55567);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute - negative feature ID is reassigned", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {3}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {3}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {-1, 1, 1, 2};
  const std::array<int32, 3> inputNumNeighbors = {0, 3, 3};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    if(i < inputNumNeighbors.size())
    {
      numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
    }
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"}));
  const std::array<int32, 4> expectedFeatureIds = {1, 1, 1, 2};
  for(usize i = 0; i < expectedFeatureIds.size(); i++)
  {
    CHECK(outputFeatureIds[i] == expectedFeatureIds[i]);
  }
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - unavailable phase (-5555)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {4}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {2, 1, 1, 3};
  const std::array<int32, 4> inputNumNeighbors = {0, 0, 3, 3};
  const std::array<int32, 4> inputPhases = {0, 1, 2, 1};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
    phases->getDataStoreRef()[i] = inputPhases[i];
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(5));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "Phases"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -5555);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - all features rejected (-55569)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {1, 2, 3, 1};
  const std::array<int32, 4> inputNumNeighbors = {0, 0, 0, 0};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(1));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -55569);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - no coarsening progress (-55572)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType cellShape = {1, 1, 4};

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {2}, imageGeom->getId());

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsStore, cellAM->getId());

  auto numNeighborsStore = DataStoreUtilities::CreateDataStore<int32>({2}, {1}, IDataAction::Mode::Execute);
  auto* numNeighbors = DataArray<int32>::Create(dataStructure, "NumNeighbors", numNeighborsStore, featureAM->getId());

  featureIds->fill(-1);
  numNeighbors->fill(0);
  numNeighbors->getDataStoreRef()[1] = 5;

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(3));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  CHECK(executeResult.result.errors()[0].code == -55572);

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    CHECK((*featureIds)[i] == -1);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RequireMinNumNeighborsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RequireMinNumNeighborsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RequireMinNumNeighborsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<RequireMinNumNeighborsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<uint64>(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key) == 5);
      CHECK(args.value<bool>(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key) == true);
      CHECK(args.value<uint64>(RequireMinNumNeighborsFilter::k_PhaseNumber_Key) == 5);
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
