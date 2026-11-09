#include "SimplnxCore/Filters/RandomizeFeatureIdsFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <array>
#include <vector>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_FeatureIdsArrayName = "Feature Ids Array";
constexpr StringLiteral k_FeatureAMName = "Feature AM";
constexpr StringLiteral k_FeatureArrayName = "Feature Array";

const DataPath k_FeatureIdsPath = DataPath{{k_FeatureIdsArrayName}};
const DataPath k_FeatureAMPath = DataPath{{k_FeatureAMName}};
const DataPath k_FeatureArrayPath = k_FeatureAMPath.createChildPath(k_FeatureArrayName);
const DataPath k_ComponentFeatureArrayPath = k_FeatureAMPath.createChildPath("Component Feature Array");

int32 scalarFeatureValue(usize oldId)
{
  return 1009 - (17 * static_cast<int32>(oldId));
}

float64 componentFeatureValue(usize oldId, usize component)
{
  return 0.25 + (101.0 * static_cast<float64>(oldId)) - (7.5 * static_cast<float64>(component));
}

void buildPermutationFixture(DataStructure& dataStructure, usize totalFeatures)
{
  const ShapeType featureShape{totalFeatures + 3};
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, featureShape);
  REQUIRE((featureAmPtr != nullptr));

  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, {3 * totalFeatures}, {1});
  auto scalarStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureArrayPath, featureShape, {1});
  auto componentStore = DataStoreUtilities::CreateDataStore<float64>(dataStructure, k_ComponentFeatureArrayPath, featureShape, {3});
  REQUIRE((idsStore != nullptr));
  REQUIRE((scalarStore != nullptr));
  REQUIRE((componentStore != nullptr));
  auto* idsPtr = Int32Array::Create(dataStructure, k_FeatureIdsArrayName, idsStore);
  auto* scalarsPtr = Int32Array::Create(dataStructure, k_FeatureArrayName, scalarStore, featureAmPtr->getId());
  auto* componentsPtr = Float64Array::Create(dataStructure, k_ComponentFeatureArrayPath.getTargetName(), componentStore, featureAmPtr->getId());
  REQUIRE((idsPtr != nullptr));
  REQUIRE((scalarsPtr != nullptr));
  REQUIRE((componentsPtr != nullptr));
  for(usize cell = 0; cell < idsPtr->getSize(); ++cell)
  {
    (*idsPtr)[cell] = static_cast<int32>(cell % totalFeatures);
  }
  for(usize oldId = 0; oldId < featureShape[0]; ++oldId)
  {
    (*scalarsPtr)[oldId] = scalarFeatureValue(oldId);
    for(usize component = 0; component < 3; ++component)
    {
      (*componentsPtr)[(3 * oldId) + component] = componentFeatureValue(oldId, component);
    }
  }
}

void requirePermutationAssociations(bool outOfCore)
{
  bool sawLongCycle = false;
  bool sawMultipleCycles = false;
  for(usize totalFeatures = 7; totalFeatures <= 64; ++totalFeatures)
  {
    CAPTURE(totalFeatures, outOfCore);
    DataStructure dataStructure;
    buildPermutationFixture(dataStructure, totalFeatures);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureArrayPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(k_ComponentFeatureArrayPath));
    const auto& ids = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
    const auto& scalars = dataStructure.getDataRefAs<Int32Array>(k_FeatureArrayPath);
    const auto& components = dataStructure.getDataRefAs<Float64Array>(k_ComponentFeatureArrayPath);
    const auto expectedStore = outOfCore ? IDataStore::StoreType::OutOfCore : IDataStore::StoreType::InMemory;
    REQUIRE((ids.getDataStoreRef().getStoreType() == expectedStore));
    REQUIRE((scalars.getDataStoreRef().getStoreType() == expectedStore));
    REQUIRE((components.getDataStoreRef().getStoreType() == expectedStore));
    if(outOfCore)
    {
      REQUIRE((ids.getDataStoreRef().getDataFormat() == "HDF5-OOC"));
      REQUIRE((scalars.getDataStoreRef().getDataFormat() == "HDF5-OOC"));
      REQUIRE((components.getDataStoreRef().getDataFormat() == "HDF5-OOC"));
    }

    const RandomizeFeatureIdsFilter filter;
    Arguments args;
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    auto execute = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

    std::vector<usize> oldToNew(totalFeatures);
    std::vector<bool> seen(totalFeatures, false);
    for(usize oldId = 0; oldId < totalFeatures; ++oldId)
    {
      REQUIRE((ids[oldId] >= 0));
      REQUIRE((static_cast<usize>(ids[oldId]) < totalFeatures));
      const usize newId = static_cast<usize>(ids[oldId]);
      REQUIRE_FALSE(seen[newId]);
      seen[newId] = true;
      oldToNew[oldId] = newId;
    }
    REQUIRE((oldToNew[0] == 0));
    std::vector<bool> visited(totalFeatures, false);
    usize nontrivialCycles = 0;
    for(usize start = 1; start < totalFeatures; ++start)
    {
      if(visited[start])
      {
        continue;
      }
      usize cycleLength = 0;
      usize current = start;
      while(!visited[current])
      {
        visited[current] = true;
        ++cycleLength;
        current = oldToNew[current];
      }
      sawLongCycle = sawLongCycle || (cycleLength > 2);
      nontrivialCycles += cycleLength > 1 ? 1 : 0;
    }
    sawMultipleCycles = sawMultipleCycles || (nontrivialCycles > 1);

    for(usize cell = 0; cell < ids.getSize(); ++cell)
    {
      const usize oldId = cell % totalFeatures;
      const usize newId = oldToNew[oldId];
      CAPTURE(cell, oldId, newId);
      REQUIRE((ids[cell] == static_cast<int32>(newId)));
      REQUIRE((scalars[newId] == scalarFeatureValue(oldId)));
      for(usize component = 0; component < 3; ++component)
      {
        CAPTURE(component);
        REQUIRE((components[(3 * newId) + component] == componentFeatureValue(oldId, component)));
      }
    }
    // Rows not indexed by any cell are outside the randomized feature population.
    for(usize row = totalFeatures; row < totalFeatures + 3; ++row)
    {
      REQUIRE((scalars[row] == scalarFeatureValue(row)));
      for(usize component = 0; component < 3; ++component)
      {
        REQUIRE((components[(3 * row) + component] == componentFeatureValue(row, component)));
      }
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
  REQUIRE((sawLongCycle));
  REQUIRE((sawMultipleCycles));
}
/**
 * | Feature Id | Count |
 * |------------|-------|
 * |     0      |   4   |
 * |     1      |   2   |
 * |     2      |   3   |
 * |     3      |   1   |
 * |     4      |   5   |
 */
constexpr std::array<int32, 15> k_OriginalFeatureIds = {0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 4, 4, 4, 4, 4};
// constexpr std::array<int32, 5> k_OriginalFeatureArray = {1, 2, 3, 4, 5};
constexpr std::array<int32, 5> k_OriginalFeatureArray = {0, 1, 2, 3, 4};

void ValidateOutput(const DataStructure& dataStructure)
{
  const auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& featureArray = dataStructure.getDataRefAs<Int32Array>(k_FeatureArrayPath);

  // Check mapping is preserved
  for(usize i = 0; i < featureIdsArray.getNumberOfTuples(); i++)
  {
    REQUIRE(featureArray[featureIdsArray[i]] == k_OriginalFeatureArray[k_OriginalFeatureIds[i]]);
  }

  // Check new array is actually sorted. Assumption: k_OriginalFeatureArray was created with an ascending sort
  usize sequentialValueCount = 0;
  for(usize i = 0; i < featureArray.getNumberOfTuples() - 1; i++)
  {
    if(featureArray[i] > featureArray[i + 1])
    {
      break;
    }
    sequentialValueCount++;
  }

  REQUIRE_FALSE(sequentialValueCount == featureArray.getNumberOfTuples() - 1);
}
} // namespace

TEST_CASE("SimplnxCore::RandomizeFeatureIdsFilter: Long cycle attribute association", "[SimplnxCore][RandomizeFeatureIdsFilter]")
{
  REQUIRE((Application::GetOrCreateInstance() != nullptr));
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  requirePermutationAssociations(false);
}

TEST_CASE("SimplnxCore::RandomizeFeatureIdsFilter: real-store long cycle attribute association", "[SimplnxCore][RandomizeFeatureIdsFilter][.OocStoreContract]")
{
  REQUIRE((Application::GetOrCreateInstance()->getIOManager("HDF5-OOC") != nullptr));
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  requirePermutationAssociations(true);
}

TEST_CASE("SimplnxCore::RandomizeFeatureIdsFilter: 5 Feature Test", "[SimplnxCore][RandomizeFeatureIdsFilter]")
{
  DataStructure dataStructure;
  {
    Int32Array* featureIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsArrayName, std::vector<usize>{15}, std::vector<usize>{1});
    (*featureIdsArray)[0] = k_OriginalFeatureIds[0];
    (*featureIdsArray)[1] = k_OriginalFeatureIds[1];
    (*featureIdsArray)[2] = k_OriginalFeatureIds[2];
    (*featureIdsArray)[3] = k_OriginalFeatureIds[3];
    (*featureIdsArray)[4] = k_OriginalFeatureIds[4];
    (*featureIdsArray)[5] = k_OriginalFeatureIds[5];
    (*featureIdsArray)[6] = k_OriginalFeatureIds[6];
    (*featureIdsArray)[7] = k_OriginalFeatureIds[7];
    (*featureIdsArray)[8] = k_OriginalFeatureIds[8];
    (*featureIdsArray)[9] = k_OriginalFeatureIds[9];
    (*featureIdsArray)[10] = k_OriginalFeatureIds[10];
    (*featureIdsArray)[11] = k_OriginalFeatureIds[11];
    (*featureIdsArray)[12] = k_OriginalFeatureIds[12];
    (*featureIdsArray)[13] = k_OriginalFeatureIds[13];
    (*featureIdsArray)[14] = k_OriginalFeatureIds[14];

    AttributeMatrix* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAMName, std::vector<usize>{5});

    Int32Array* featureArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureArrayName, featureAM->getShape(), std::vector<usize>{1}, featureAM->getId());
    (*featureArray)[0] = k_OriginalFeatureArray[0];
    (*featureArray)[1] = k_OriginalFeatureArray[1];
    (*featureArray)[2] = k_OriginalFeatureArray[2];
    (*featureArray)[3] = k_OriginalFeatureArray[3];
    (*featureArray)[4] = k_OriginalFeatureArray[4];
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RandomizeFeatureIdsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ValidateOutput(dataStructure);
}

TEST_CASE("SimplnxCore::RandomizeFeatureIdsFilter: 4 Feature Test", "[SimplnxCore][RandomizeFeatureIdsFilter]")
{
  DataStructure dataStructure;
  {
    Int32Array* featureIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsArrayName, std::vector<usize>{10}, std::vector<usize>{1});
    (*featureIdsArray)[0] = k_OriginalFeatureIds[0];
    (*featureIdsArray)[1] = k_OriginalFeatureIds[1];
    (*featureIdsArray)[2] = k_OriginalFeatureIds[2];
    (*featureIdsArray)[3] = k_OriginalFeatureIds[3];
    (*featureIdsArray)[4] = k_OriginalFeatureIds[4];
    (*featureIdsArray)[5] = k_OriginalFeatureIds[5];
    (*featureIdsArray)[6] = k_OriginalFeatureIds[6];
    (*featureIdsArray)[7] = k_OriginalFeatureIds[7];
    (*featureIdsArray)[8] = k_OriginalFeatureIds[8];
    (*featureIdsArray)[9] = k_OriginalFeatureIds[9];

    AttributeMatrix* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAMName, std::vector<usize>{4});

    Int32Array* featureArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureArrayName, featureAM->getShape(), std::vector<usize>{1}, featureAM->getId());
    (*featureArray)[0] = k_OriginalFeatureArray[0];
    (*featureArray)[1] = k_OriginalFeatureArray[1];
    (*featureArray)[2] = k_OriginalFeatureArray[2];
    (*featureArray)[3] = k_OriginalFeatureArray[3];
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RandomizeFeatureIdsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ValidateOutput(dataStructure);
}
