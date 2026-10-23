#include "FeatureRemovalTestUtils.hpp"
#include "SimplnxCore/Filters/ComputeFeatureNeighborsFilter.hpp"
#include "SimplnxCore/Filters/RequireMinNumNeighborsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include <any>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
/**
 * @namespace DiscriminatingFixture
 * @brief Defines an analytical coarsening fixture with boundary and tie cases.
 */
namespace DiscriminatingFixture
{
constexpr usize k_Dimension = 6;
constexpr usize k_CellCount = k_Dimension * k_Dimension * k_Dimension;
constexpr usize k_FeatureCount = 6;

/**
 * @struct Coordinate
 * @brief Stores one zero-based cell coordinate in the fixture volume.
 */
struct Coordinate
{
  usize x = 0;
  usize y = 0;
  usize z = 0;

  /**
   * @brief Tests whether this coordinate equals selected axis values.
   * @param pointX X coordinate to test.
   * @param pointY Y coordinate to test.
   * @param pointZ Z coordinate to test.
   * @return True if all three coordinates are equal.
   */
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

/**
 * @brief Converts fixture coordinates to a row-major cell index.
 * @param x Zero-based X coordinate.
 * @param y Zero-based Y coordinate.
 * @param z Zero-based Z coordinate.
 * @return Linear cell index.
 */
constexpr usize GetIndex(usize x, usize y, usize z)
{
  return z * k_Dimension * k_Dimension + y * k_Dimension + x;
}

/**
 * @brief Converts a fixture Coordinate to a row-major cell index.
 * @param coordinate Coordinate to convert.
 * @return Linear cell index.
 */
constexpr usize GetIndex(const Coordinate& coordinate)
{
  return GetIndex(coordinate.x, coordinate.y, coordinate.z);
}

/**
 * @brief Tests whether a cell belongs to the rejected 3-cubed center feature.
 * @param x Zero-based X coordinate.
 * @param y Zero-based Y coordinate.
 * @param z Zero-based Z coordinate.
 * @return True for a cell in the center cube.
 */
constexpr bool IsRejectedCubeCell(usize x, usize y, usize z)
{
  return x >= 1 && x <= 3 && y >= 1 && y <= 3 && z >= 1 && z <= 3;
}

/**
 * @brief Tests whether a cell belongs to any rejected fixture location.
 * @param x Zero-based X coordinate.
 * @param y Zero-based Y coordinate.
 * @param z Zero-based Z coordinate.
 * @return True for a rejected cell.
 */
constexpr bool IsRejectedCell(usize x, usize y, usize z)
{
  return IsRejectedCubeCell(x, y, z) || k_TieCell.isAt(x, y, z) || k_TieNegativeXCell.isAt(x, y, z) || k_PositiveXBoundaryCell.isAt(x, y, z) || k_PositiveYBoundaryCell.isAt(x, y, z);
}

/**
 * @brief Calculates the input feature identifier at one fixture cell.
 * @param x Zero-based X coordinate.
 * @param y Zero-based Y coordinate.
 * @param z Zero-based Z coordinate.
 * @return Input feature identifier.
 */
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

/**
 * @brief Calculates the source tuple selected by deterministic coarsening.
 * @param x Zero-based X coordinate.
 * @param y Zero-based Y coordinate.
 * @param z Zero-based Z coordinate.
 * @return Linear source tuple index after all coarsening passes.
 */
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

/**
 * @brief Calculates the compacted feature identifier at one output cell.
 * @param x Zero-based X coordinate.
 * @param y Zero-based Y coordinate.
 * @param z Zero-based Z coordinate.
 * @return Expected output feature identifier.
 */
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

/**
 * @brief Builds the discriminating fixture and all expected source arrays.
 * @param dataStructure Receives the geometry, cell arrays, and feature arrays.
 */
void PopulateDataStructure(DataStructure& dataStructure)
{
  const SizeVec3 imageSize = {k_Dimension, k_Dimension, k_Dimension};
  const ShapeType cellShape = {k_Dimension, k_Dimension, k_Dimension};

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {k_FeatureCount}, imageGeom->getId());

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"ImageGeometry", "CellData", "FeatureIds"}), cellShape, {1});
  auto* featureIds = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsStore, cellAM->getId());
  auto copiedScalarStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"ImageGeometry", "CellData", "CopiedScalar"}), cellShape, {1});
  auto* copiedScalar = DataArray<int32>::Create(dataStructure, "CopiedScalar", copiedScalarStore, cellAM->getId());
  auto copiedVectorStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, DataPath({"ImageGeometry", "CellData", "CopiedVector"}), cellShape, {3});
  auto* copiedVector = DataArray<float32>::Create(dataStructure, "CopiedVector", copiedVectorStore, cellAM->getId());
  auto ignoredValuesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"ImageGeometry", "CellData", "IgnoredValues"}), cellShape, {1});
  auto* ignoredValues = DataArray<int32>::Create(dataStructure, "IgnoredValues", ignoredValuesStore, cellAM->getId());
  auto numNeighborsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"}), {k_FeatureCount}, {1});
  auto* numNeighbors = DataArray<int32>::Create(dataStructure, "NumNeighbors", numNeighborsStore, featureAM->getId());
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"ImageGeometry", "FeatureData", "Phases"}), {k_FeatureCount}, {1});
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

/**
 * @brief Selects the analytical fixture inputs for array-kind validation.
 * @return Arguments with all three numeric inputs active.
 */
Arguments createArrayKindValidationArguments()
{
  Arguments args = RequireMinNumNeighborsFilter().getDefaultArguments();
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, DataPath({"ImageGeometry"}));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, DataPath({"ImageGeometry", "CellData", "FeatureIds"}));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"}));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, DataPath({"ImageGeometry", "FeatureData", "Phases"}));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, true);
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, uint64{1});
  return args;
}
} // namespace

// The tests use two analytical oracles.
//
// The 4 by 1 by 1 oracle starts with FeatureIds [2, 1, 1, 3]. Feature 1 is rejected.
// Its left cell has feature 2 as its only retained neighbor. Its right cell has feature 3.
// The copied scalar values are therefore [20, 20, 30, 30].
// Compaction maps features 2 and 3 to identifiers 1 and 2.
// The final FeatureIds are [1, 1, 2, 2].
//
// The 6-cubed oracle rejects a 3-cubed feature-1 region and four additional cells.
// Thus, 31 cells are initially rejected. The first pass fills the cube surface and additional cells.
// Only the cube center remains for the second pass. The observed coarsening counts are 31, 1, and 0.
// Its final positive-Z neighbor supplies a tuple copied from exterior cell (2, 2, 4).
//
// The tie cell has feature 3 at negative Z and feature 4 at negative Y.
// Face traversal order is negative Z, negative Y, negative X, positive X, positive Y, and positive Z.
// A source changes only when its vote count exceeds the current maximum.
// Thus, a one-to-one tie keeps the first vote and selects feature 3.
// For matching exterior features, the last valid neighbor supplies the tuple.
// GetExpectedSourceIndex() encodes this traversal rule.
//
// Feature compaction maps retained input features 2, 3, and 4 to identifiers 1, 2, and 3.
// Feature 5 owns no cells. All-phase mode removes it.
// Single-phase mode retains feature 5 because it belongs to another phase.
// The feature-array lengths are therefore 4 and 5, while all cell arrays remain equal.
// Expected scalar, vector, NumNeighbors, and Phases values follow from the source indices and compaction mapping.

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle", "[SimplnxCore][RequireMinNumNeighborsFilter]")
{
  UnitTest::LoadPlugins();

  const bool singlePhase = GENERATE(false, true);
  DYNAMIC_SECTION("ApplyToSinglePhase=" << singlePhase)
  {
    // Feature 1 occupies the two middle cells of [2, 1, 1, 3] and is rejected.
    // Its left and right face-neighbor votes select features 2 and 3.
    // Inactive-feature removal then produces FeatureIds [1, 1, 2, 2].
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

    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

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
      // Reassignment removes all rejected identifiers and keeps each identifier in range.
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

    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

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
  // Load the Small IN100 exemplar before each selected algorithm scenario.
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

    // Preflight must accept the configured feature arrays.
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execution must complete the selected feature-coarsening scenario.
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

    // Preflight must accept the configured feature arrays.
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    CHECK(preflightResult.outputActions.warnings()[0].code == -5558);

    // Execution must complete the selected feature-coarsening scenario.
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
  // The optional output supports manual inspection of the coarsened arrays.
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/minimum_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Single-phase preflight requires equal tuple counts for NumNeighbors and FeaturePhases.
  // Separate AttributeMatrix objects give these arrays different tuple counts.
  // This mismatch reaches the validateNumberOfTuples() guard for error -252.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({4, 1, 1});

  // FeatureIds must exist for preflight, but it is not part of the feature-level tuple-count check.
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 4}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 1, 4}, {1}, cellAM->getId());

  // NumNeighbors has five feature tuples.
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {5}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {5}, {1}, featureAM->getId());

  // FeaturePhases has four tuples in a separate AttributeMatrix.
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

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Selected cell tuple counts", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();
  const usize tupleCount = GENERATE(usize{3}, usize{4}, usize{5});
  CAPTURE(tupleCount);

  DataStructure dataStructure;
  auto* selectedGroupPtr = DataGroup::Create(dataStructure, "Selected");
  REQUIRE(selectedGroupPtr != nullptr);
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry", selectedGroupPtr->getId());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 1});
  // Equal counts are valid in the geometry's Cell Attribute Matrix even when
  // its tuple shape differs from the geometry's {1, 2, 2} shape.
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {tupleCount}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);
  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeomPtr->getId());
  REQUIRE(featureAMPtr != nullptr);
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {tupleCount}, {1}, cellAMPtr->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAMPtr->getId());
  const DataPath geometryPath({"Selected", "ImageGeometry"});
  const DataPath featureIdsPath({"Selected", "ImageGeometry", "CellData", "FeatureIds"});

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(geometryPath.createChildPath("FeatureData").createChildPath("NumNeighbors")));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  if(tupleCount == 4)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    const auto& error = preflightResult.outputActions.errors()[0];
    CHECK(error.code == -59571);
    CHECK(error.message.find(featureIdsPath.toString()) != std::string::npos);
    CHECK(error.message.find(geometryPath.toString()) != std::string::npos);
    CHECK(error.message.find(fmt::format("has {} tuples", tupleCount)) != std::string::npos);
    CHECK(error.message.find("has 4 cells") != std::string::npos);
    CHECK(error.message.find("one tuple per geometry cell") != std::string::npos);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds outside Cell Attribute Matrix (-55575)", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "OtherData", {1, 1, 4}, imageGeomPtr->getId());
  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeomPtr->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 1, 4}, {1}, otherAMPtr->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAMPtr->getId());

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "OtherData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -55575);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Rejects unsupported array kinds", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();
  const usize inputIndex = GENERATE(usize{0}, usize{1}, usize{2});
  const bool useStringArray = GENERATE(false, true);
  CAPTURE(inputIndex, useStringArray);
  const std::array<StringLiteral, 3> inputKeys = {RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key,
                                                  RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key};
  constexpr std::array<int32, 3> expectedCodes = {-59574, -59575, -59576};
  DataStructure dataStructure;
  DiscriminatingFixture::PopulateDataStructure(dataStructure);
  Arguments args = createArrayKindValidationArguments();
  const auto parentPath = args.value<DataPath>(inputKeys.at(inputIndex)).getParent();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(parentPath));
  const auto& parent = dataStructure.getDataRefAs<AttributeMatrix>(parentPath);
  const DataPath rejectedPath = parentPath.createChildPath("RejectedInput");
  if(useStringArray)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, rejectedPath.getTargetName(), parent.getShape(), std::vector<std::string>(parent.getNumberOfTuples(), "17"), parent.getId()) != nullptr);
  }
  else
  {
    auto* listsPtr = NeighborList<int32>::Create(dataStructure, rejectedPath.getTargetName(), parent.getShape(), parent.getId());
    REQUIRE(listsPtr != nullptr);
    for(usize index = 0; index < parent.getNumberOfTuples(); index++)
    {
      listsPtr->setList(static_cast<int32>(index), std::vector<int32>{17});
    }
    REQUIRE(listsPtr->getTupleShape() == parent.getShape());
  }
  args.insertOrAssign(inputKeys.at(inputIndex), rejectedPath);
  const auto initialPaths = dataStructure.getAllDataPaths();
  const auto initialNextId = dataStructure.getNextId();
  const RequireMinNumNeighborsFilter filter;
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  const auto& error = preflightResult.outputActions.errors().front();
  CHECK(error.code == expectedCodes.at(inputIndex));
  CHECK(error.message.find(rejectedPath.toString()) != std::string::npos);
  CHECK(error.message.find(useStringArray ? "StringArray" : "NeighborList<int32>") != std::string::npos);
  CHECK(error.message.find("scalar int32 DataArray") != std::string::npos);
  CHECK(error.message.find("Select a scalar int32 DataArray") != std::string::npos);
  CHECK(dataStructure.getAllDataPaths() == initialPaths);
  CHECK(dataStructure.getNextId() == initialNextId);
  if(useStringArray)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(rejectedPath));
    CHECK(dataStructure.getDataRefAs<StringArray>(rejectedPath)[0] == "17");
  }
  else
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(rejectedPath));
    CHECK(dataStructure.getDataRefAs<NeighborList<int32>>(rejectedPath).getList(0) == std::vector<int32>{17});
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "CopiedScalar"})));
  CHECK(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "CopiedScalar"}))[0] == 10000);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Inactive phases skip array-kind validation", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();
  const usize inactiveKind = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(inactiveKind);
  DataStructure dataStructure;
  DiscriminatingFixture::PopulateDataStructure(dataStructure);
  const DataPath inactivePath({"InactivePhases"});
  if(inactiveKind == 0)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, inactivePath.getTargetName(), {1}, {"unused"}) != nullptr);
  }
  else if(inactiveKind == 1)
  {
    REQUIRE(NeighborList<int32>::Create(dataStructure, inactivePath.getTargetName(), {1}) != nullptr);
  }
  Arguments args = createArrayKindValidationArguments();
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, false);
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, inactivePath);
  const auto initialPaths = dataStructure.getAllDataPaths();
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = RequireMinNumNeighborsFilter().preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  CHECK(dataStructure.getAllDataPaths() == initialPaths);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Tuple errors precede feature array-kind errors", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();
  const bool replaceNeighbors = GENERATE(false, true);
  const bool useStringArray = GENERATE(false, true);
  const bool cellMismatch = GENERATE(false, true);
  CAPTURE(replaceNeighbors, useStringArray, cellMismatch);
  DataStructure dataStructure;
  DiscriminatingFixture::PopulateDataStructure(dataStructure);
  const auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"ImageGeometry", "FeatureData"}));
  const DataPath rejectedPath({"ImageGeometry", "FeatureData", "RejectedFeatureInput"});
  if(useStringArray)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, rejectedPath.getTargetName(), featureAM.getShape(), std::vector<std::string>(featureAM.getNumberOfTuples(), "17"), featureAM.getId()) !=
            nullptr);
  }
  else
  {
    REQUIRE(NeighborList<int32>::Create(dataStructure, rejectedPath.getTargetName(), featureAM.getShape(), featureAM.getId()) != nullptr);
  }
  // Keep the other selected feature array in a separate AM so its tuple count can differ.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({"ImageGeometry"}));
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {5}, imageGeom.getId());
  REQUIRE(mismatchAM != nullptr);
  const DataPath mismatchPath({"ImageGeometry", "MismatchData", "OtherFeatureInput"});
  UnitTest::CreateTestDataArray<int32>(dataStructure, mismatchPath.getTargetName(), {5}, {1}, mismatchAM->getId());
  if(cellMismatch)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({"ImageGeometry"})));
    dataStructure.getDataRefAs<ImageGeom>(DataPath({"ImageGeometry"})).setDimensions({7, 6, 6});
  }
  Arguments args = createArrayKindValidationArguments();
  args.insertOrAssign(replaceNeighbors ? RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key : RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, rejectedPath);
  args.insertOrAssign(replaceNeighbors ? RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key : RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, mismatchPath);
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = RequireMinNumNeighborsFilter().preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  CHECK(preflightResult.outputActions.errors().front().code == (cellMismatch ? -59571 : -252));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
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

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
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

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
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

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
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

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"ImageGeometry", "CellData", "FeatureIds"}), cellShape, {1});
  auto* featureIds = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsStore, cellAM->getId());

  auto numNeighborsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"}), {2}, {1});
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

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Strict Feature Ids", "[SimplnxCore][RequireMinNumNeighborsFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath geometryPath({"Strict Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cells");
  const DataPath featurePath = geometryPath.createChildPath("Features");
  const DataPath idsPath = cellPath.createChildPath("Ids");
  const DataPath otherPath = geometryPath.createChildPath("Other Cells").createChildPath("Other Values");
  const ShapeType shape{1, 1, 4};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {4}, imageGeomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  auto* otherPtr = AttributeMatrix::Create(dataStructure, "Other Cells", shape, imageGeomPtr->getId());
  REQUIRE(otherPtr != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cellsPtr->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Other Values", shape, {1}, otherPtr->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Values", {4}, {1}, featuresPtr->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Cell Feature Values", shape, {1}, cellsPtr->getId()) != nullptr);
  auto* elsewherePtr = DataGroup::Create(dataStructure, "Elsewhere");
  REQUIRE(elsewherePtr != nullptr);
  auto* externalFeaturesPtr = AttributeMatrix::Create(dataStructure, "External Features", {4}, elsewherePtr->getId());
  REQUIRE(externalFeaturesPtr != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Values", {4}, {1}, externalFeaturesPtr->getId()) != nullptr);

  RequireMinNumNeighborsFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Feature Values")));

  int32 expectedCode = 0;
  DataPath offendingPath;
  SECTION("Correct Cell and Feature Attribute Matrices")
  {
  }
  SECTION("Feature Ids in a second Attribute Matrix with the same tuple shape")
  {
    offendingPath = otherPath;
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -55575;
  }
  SECTION("Feature array outside the selected geometry")
  {
    offendingPath = DataPath({"Elsewhere", "External Features", "Feature Values"});
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -55574;
  }
  SECTION("Feature array under a plain DataGroup")
  {
    REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Ungrouped Features", {4}, {1}, elsewherePtr->getId()) != nullptr);
    offendingPath = DataPath({"Elsewhere", "Ungrouped Features"});
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -55574;
  }
  SECTION("Feature array in the Cell Attribute Matrix")
  {
    offendingPath = cellPath.createChildPath("Cell Feature Values");
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -55574;
  }
  SECTION("Feature Ids through a shared Cell Attribute Matrix")
  {
    REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), elsewherePtr->getId()));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Elsewhere", "Cells", "Ids"})));
  }
  SECTION("Ignored array in a second Attribute Matrix")
  {
    offendingPath = otherPath;
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{otherPath}));
    expectedCode = -55573;
  }
  SECTION("Read-only Feature Phases in another Attribute Matrix")
  {
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(true));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(otherPath));
  }
  const auto result = filter.preflight(dataStructure, args);
  if(expectedCode == 0)
  {
    REQUIRE(result.outputActions.valid());
    for(const auto& modification : result.outputActions.value().modifiedActions)
    {
      CHECK(modification.modifiedPath.getParent() != DataPath({"Elsewhere", "Cells"}));
    }
  }
  else
  {
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == expectedCode);
    CHECK(result.outputActions.errors().front().message.find(offendingPath.toString()) != std::string::npos);
    CHECK(result.outputActions.errors().front().message.find(cellPath.toString()) != std::string::npos);
    CHECK(result.outputActions.errors().front().message.find(geometryPath.toString()) != std::string::npos);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Shared Cell Paths and Selected Feature Matrix", "[SimplnxCore][RequireMinNumNeighborsFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  const bool useAliasForIds = GENERATE(false, true);
  CAPTURE(useAliasForIds);
  DataStructure dataStructure;
  const DataPath geometryPath({"Alias Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cells");
  const DataPath aliasCellPath({"Earlier Parent", "Cells"});
  const DataPath featurePath = geometryPath.createChildPath("Features");
  const ShapeType shape{1, 1, 4};
  auto* earlierParentPtr = DataGroup::Create(dataStructure, "Earlier Parent");
  REQUIRE(earlierParentPtr != nullptr);
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), earlierParentPtr->getId()));
  REQUIRE(imageGeomPtr->getCellDataPath() == cellPath);
  auto* idsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cellsPtr->getId());
  auto* valuesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Values", shape, {1}, cellsPtr->getId());
  auto* ignoredPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ignored", shape, {1}, cellsPtr->getId());
  REQUIRE(idsPtr != nullptr);
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(ignoredPtr != nullptr);
  const std::vector<int32> originalIds{1, 2, 1, 1};
  for(usize index = 0; index < 4; index++)
  {
    (*idsPtr)[index] = originalIds[index];
    (*valuesPtr)[index] = static_cast<int32>(10 + index);
    (*ignoredPtr)[index] = static_cast<int32>(100 + index);
  }

  // The selected array has another parent; only the selected Feature Attribute Matrix is compacted.
  auto* featureValuesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Values", {3}, {1}, earlierParentPtr->getId());
  auto* externalMarkerPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "External Marker", {3}, {1}, earlierParentPtr->getId());
  REQUIRE(featureValuesPtr != nullptr);
  REQUIRE(externalMarkerPtr != nullptr);
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {3}, imageGeomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(featureValuesPtr->getId(), featuresPtr->getId()));
  auto* markerPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Marker", {3}, {1}, featuresPtr->getId());
  REQUIRE(markerPtr != nullptr);
  for(usize index = 0; index < 3; index++)
  {
    (*markerPtr)[index] = static_cast<int32>(11 * index);
    (*externalMarkerPtr)[index] = static_cast<int32>(90 + index);
  }
  (*featureValuesPtr)[0] = 0;
  (*featureValuesPtr)[1] = 3;
  (*featureValuesPtr)[2] = 1;

  RequireMinNumNeighborsFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>((useAliasForIds ? aliasCellPath : cellPath).createChildPath("Ids")));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Feature Values")));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
  const auto ignoreParent = useAliasForIds ? cellPath : aliasCellPath;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{ignoreParent.createChildPath("Ignored")}));
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  bool reportsValues = false;
  for(const auto& modification : preflight.outputActions.value().modifiedActions)
  {
    CHECK(modification.modifiedPath.getParent() != aliasCellPath);
    reportsValues = reportsValues || modification.modifiedPath == cellPath.createChildPath("Values");
  }
  CHECK(reportsValues);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  for(usize index = 0; index < 4; index++)
  {
    CHECK((*idsPtr)[index] == 1);
    CHECK((*ignoredPtr)[index] == static_cast<int32>(100 + index));
    if(index != 1)
    {
      CHECK((*valuesPtr)[index] == static_cast<int32>(10 + index));
    }
  }
  CHECK(((*valuesPtr)[1] == 10 || (*valuesPtr)[1] == 12));
  CHECK(featuresPtr->getNumberOfTuples() == 2);
  REQUIRE(markerPtr->getNumberOfTuples() == 2);
  CHECK((*markerPtr)[0] == 0);
  CHECK((*markerPtr)[1] == 11);
  REQUIRE(externalMarkerPtr->getNumberOfTuples() == 3);
  for(usize index = 0; index < 3; index++)
  {
    CHECK((*externalMarkerPtr)[index] == static_cast<int32>(90 + index));
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
      // Successful pipeline loading verifies the MultiDataArraySelectionFilterParameterConverter value.
    }
  }
}
