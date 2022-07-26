#include <iostream>
#include <random>
#include <string>

#include <catch2/catch.hpp>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/MinNeighbors.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
inline const std::vector<usize> k_Dimensions{5, 5, 5};
inline const std::vector<usize> k_CompShape{1};

inline const std::string k_ImageGeomName = "Image Geom";
inline const std::string k_FeaturePhasesName = "Feature Phases";
inline const std::string k_FeatureIdsName = "Feature IDs";
inline const std::string k_NumNeighborsName = "Num Neighbors";
inline const std::vector<std::string> k_VoxelArrayNames = {"Array1", "Array2", "Array3"};
inline const std::vector<DataPath> k_VoxelArrayPaths = {DataPath({k_ImageGeomName, "Array1"}), DataPath({k_ImageGeomName, "Array2"}), DataPath({k_ImageGeomName, "Array3"})};

DataStructure createTestData()
{
  DataStructure data;

  auto* imageGeom = ImageGeom::Create(data, k_ImageGeomName);
  imageGeom->setDimensions({5, 5, 5});
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});

  std::vector<std::string> arrayNames = k_VoxelArrayNames;
  arrayNames.push_back(k_FeaturePhasesName);
  arrayNames.push_back(k_FeatureIdsName);
  arrayNames.push_back(k_NumNeighborsName);

  for(const auto& arrayName : arrayNames)
  {
    auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(data, arrayName, k_Dimensions, k_CompShape, imageGeom->getId());
    dataArray->fill(0);
  }

  // Generate Feature IDs
  {
    std::random_device rand;
    std::mt19937 generator(rand());
    std::uniform_int_distribution<int32> uni(0, 1024);

    int32 maxFeatureId = 0;

    auto& featureIdsArray = data.getDataRefAs<Int32Array>(DataPath({k_ImageGeomName, k_FeatureIdsName}));
    auto& featureIds = featureIdsArray.getDataStoreRef();
    auto& featurePhasesArray = data.getDataRefAs<Int32Array>(DataPath({k_ImageGeomName, k_FeaturePhasesName}));
    auto& featurePhases = featurePhasesArray.getDataStoreRef();

    for(usize i = 0; i < featureIds.getSize(); ++i)
    {
      int32 value = uni(generator) % (maxFeatureId + 1);
      featureIds[i] = value;
      featurePhases[i] = value;
      maxFeatureId = value > maxFeatureId ? value : maxFeatureId;
    }
  }

  {
    int32 neighborCount = 6;
    auto& numNeighborsArray = data.getDataRefAs<Int32Array>(DataPath({k_ImageGeomName, k_NumNeighborsName}));
    auto& numNeighbors = numNeighborsArray.getDataStoreRef();
    numNeighbors.fill(neighborCount);
  }

  return data;
}
} // namespace

TEST_CASE("ComplexCore::MinNeighbors: Phase 0", "[MinNeighbors]")
{
  MinNeighbors filter;
  DataStructure dataGraph = createTestData();
  Arguments args;

  DataPath k_ImageGeomPath({k_ImageGeomName});
  bool k_ApplyToSinglePhase = true;
  uint64 k_PhaseNumber = 0;
  DataPath k_FeatureIdsPath({k_ImageGeomName, k_FeatureIdsName});
  DataPath k_FeaturePhases({k_ImageGeomName, k_FeaturePhasesName});
  DataPath k_NumNeighbors({k_ImageGeomName, k_NumNeighborsName});
  uint64 k_MinNumNeighbors = 1;
  std::vector<DataPath> k_VoxelArrays = k_VoxelArrayPaths;

  args.insertOrAssign(MinNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(MinNeighbors::k_ApplyToSinglePhase_Key, std::make_any<bool>(k_ApplyToSinglePhase));
  args.insertOrAssign(MinNeighbors::k_PhaseNumber_Key, std::make_any<uint64>(k_PhaseNumber));
  args.insertOrAssign(MinNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(MinNeighbors::k_FeaturePhases_Key, std::make_any<DataPath>(k_FeaturePhases));
  args.insertOrAssign(MinNeighbors::k_NumNeighbors_Key, std::make_any<DataPath>(k_NumNeighbors));
  args.insertOrAssign(MinNeighbors::k_MinNumNeighbors_Key, std::make_any<uint64>(k_MinNumNeighbors));
  args.insertOrAssign(MinNeighbors::k_VoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& featureIdsArray = dataGraph.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  auto& featureIds = featureIdsArray.getDataStoreRef();
  for(usize i = 0; i < featureIds.getSize(); ++i)
  {
    REQUIRE(featureIds[i] == 0);
  }
}

TEST_CASE("ComplexCore::MinNeighbors: Bad Phase Number", "[MinNeighbors]")
{
  MinNeighbors filter;
  DataStructure dataGraph = createTestData();
  Arguments args;

  DataPath k_ImageGeomPath({k_ImageGeomName});
  bool k_ApplyToSinglePhase = true;
  uint64 k_PhaseNumber = 500;
  DataPath k_FeatureIdsPath({k_ImageGeomName, k_FeatureIdsName});
  DataPath k_FeaturePhases({k_ImageGeomName, k_FeaturePhasesName});
  DataPath k_NumNeighbors({k_ImageGeomName, k_NumNeighborsName});
  uint64 k_MinNumNeighbors = 1;
  std::vector<DataPath> k_VoxelArrays = k_VoxelArrayPaths;

  args.insertOrAssign(MinNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(MinNeighbors::k_ApplyToSinglePhase_Key, std::make_any<bool>(k_ApplyToSinglePhase));
  args.insertOrAssign(MinNeighbors::k_PhaseNumber_Key, std::make_any<uint64>(k_PhaseNumber));
  args.insertOrAssign(MinNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(MinNeighbors::k_FeaturePhases_Key, std::make_any<DataPath>(k_FeaturePhases));
  args.insertOrAssign(MinNeighbors::k_NumNeighbors_Key, std::make_any<DataPath>(k_NumNeighbors));
  args.insertOrAssign(MinNeighbors::k_MinNumNeighbors_Key, std::make_any<uint64>(k_MinNumNeighbors));
  args.insertOrAssign(MinNeighbors::k_VoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
}

TEST_CASE("ComplexCore::MinNeighbors: Phase Array", "[MinNeighbors]")
{
  MinNeighbors filter;
  DataStructure dataGraph = createTestData();
  Arguments args;

  DataPath k_ImageGeomPath({k_ImageGeomName});
  bool k_ApplyToSinglePhase = false;
  uint64 k_PhaseNumber = 500;
  DataPath k_FeatureIdsPath({k_ImageGeomName, k_FeatureIdsName});
  DataPath k_FeaturePhases({k_ImageGeomName, k_FeaturePhasesName});
  DataPath k_NumNeighbors({k_ImageGeomName, k_NumNeighborsName});
  uint64 k_MinNumNeighbors = 1;
  std::vector<DataPath> k_VoxelArrays = k_VoxelArrayPaths;

  args.insertOrAssign(MinNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(MinNeighbors::k_ApplyToSinglePhase_Key, std::make_any<bool>(k_ApplyToSinglePhase));
  args.insertOrAssign(MinNeighbors::k_PhaseNumber_Key, std::make_any<uint64>(k_PhaseNumber));
  args.insertOrAssign(MinNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(MinNeighbors::k_FeaturePhases_Key, std::make_any<DataPath>(k_FeaturePhases));
  args.insertOrAssign(MinNeighbors::k_NumNeighbors_Key, std::make_any<DataPath>(k_NumNeighbors));
  args.insertOrAssign(MinNeighbors::k_MinNumNeighbors_Key, std::make_any<uint64>(k_MinNumNeighbors));
  args.insertOrAssign(MinNeighbors::k_VoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
}
