#include "FeatureNeighborOrientationTestUtils.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <catch2/catch.hpp>

#include <algorithm>
#include <memory>
#include <vector>

namespace nx::core::UnitTest::FeatureNeighbors
{
InputPaths::InputPaths(const DataPath& featureMatrix, const DataPath& ensembleMatrix)
: featurePhases(featureMatrix.createChildPath("FeaturePhases"))
, avgQuats(featureMatrix.createChildPath("AvgQuats"))
, neighborList(featureMatrix.createChildPath("NeighborList"))
, crystalStructures(ensembleMatrix.createChildPath("CrystalStructures"))
{
}

void InitializeFeatures(Int32Array& phases, Float32Array& quats, NeighborList<int32>& neighbors)
{
  const usize numFeatures = phases.getNumberOfTuples();
  for(usize i = 0; i < numFeatures; ++i)
  {
    phases[i] = 0;
    quats[i * 4 + 0] = 0.0f;
    quats[i * 4 + 1] = 0.0f;
    quats[i * 4 + 2] = 0.0f;
    quats[i * 4 + 3] = 1.0f;
    neighbors.setList(i, std::make_shared<std::vector<int32>>());
  }
}

void CreateChunkTailInputs(DataStructure& dataStructure, const InputPaths& paths, uint32 crystalStructure, const std::array<float32, 4>& lastFullChunkQuat, const std::array<float32, 4>& tailQuat)
{
  constexpr usize k_FeatureTuples = k_TailTuple + 1;
  const DataPath featureMatrixPath = paths.featurePhases.getParent();
  auto* dataGroup = DataGroup::Create(dataStructure, featureMatrixPath.getParent().getTargetName());
  REQUIRE(dataGroup != nullptr);
  auto* featureData = AttributeMatrix::Create(dataStructure, featureMatrixPath.getTargetName(), {k_FeatureTuples}, dataGroup->getId());
  auto* ensembleData = AttributeMatrix::Create(dataStructure, paths.crystalStructures.getParent().getTargetName(), {2}, dataGroup->getId());
  REQUIRE(featureData != nullptr);
  REQUIRE(ensembleData != nullptr);

  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, paths.featurePhases, {k_FeatureTuples}, {1});
  auto avgQuatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, paths.avgQuats, {k_FeatureTuples}, {4});
  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, paths.crystalStructures, {2}, {1});
  auto* featurePhases = Int32Array::Create(dataStructure, paths.featurePhases.getTargetName(), featurePhasesStore, featureData->getId());
  auto* avgQuats = Float32Array::Create(dataStructure, paths.avgQuats.getTargetName(), avgQuatsStore, featureData->getId());
  auto* neighborList = NeighborList<int32>::Create(dataStructure, paths.neighborList.getTargetName(), {k_FeatureTuples}, featureData->getId());
  auto* crystalStructures = UInt32Array::Create(dataStructure, paths.crystalStructures.getTargetName(), crystalStructuresStore, ensembleData->getId());
  REQUIRE(featurePhases != nullptr);
  REQUIRE(avgQuats != nullptr);
  REQUIRE(neighborList != nullptr);
  REQUIRE(crystalStructures != nullptr);

  // Write the resident fixture buffers in blocks, including the partial final chunk.
  constexpr usize k_BlockTuples = 1024;
  std::array<int32, k_BlockTuples> phases;
  phases.fill(1);
  std::array<float32, 4 * k_BlockTuples> quats{};
  for(usize i = 3; i < quats.size(); i += 4)
  {
    quats[i] = 1.0F;
  }
  for(usize start = 0; start < k_FeatureTuples; start += k_BlockTuples)
  {
    const usize count = std::min(k_BlockTuples, k_FeatureTuples - start);
    phases[0] = start == 0 ? 0 : 1;
    const auto phaseWrite = featurePhases->getDataStoreRef().copyFromBuffer(start, nonstd::span<const int32>(phases.data(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(phaseWrite);
    const auto quatWrite = avgQuats->getDataStoreRef().copyFromBuffer(start * 4, nonstd::span<const float32>(quats.data(), count * 4));
    SIMPLNX_RESULT_REQUIRE_VALID(quatWrite);
  }
  const auto lastFullWrite = avgQuats->getDataStoreRef().copyFromBuffer(k_LastFullChunkTuple * 4, nonstd::span<const float32>(lastFullChunkQuat));
  SIMPLNX_RESULT_REQUIRE_VALID(lastFullWrite);
  const auto tailWrite = avgQuats->getDataStoreRef().copyFromBuffer(k_TailTuple * 4, nonstd::span<const float32>(tailQuat));
  SIMPLNX_RESULT_REQUIRE_VALID(tailWrite);
  const std::array<uint32, 2> crystalStructureValues = {ebsdlib::CrystalStructure::UnknownCrystalStructure, crystalStructure};
  auto crystalStructuresWriteResult = crystalStructures->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(crystalStructureValues.data(), crystalStructureValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(crystalStructuresWriteResult);

  neighborList->setList(k_LastFullChunkTuple, std::vector<int32>{1});
  neighborList->setList(k_TailTuple, std::vector<int32>{1});

  REQUIRE(featurePhases->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(avgQuats->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  const auto quaternionMetadata = avgQuats->getDataStoreRef().getRecoveryMetadata();
  REQUIRE(quaternionMetadata.at("OocChunkShape") == "65536");
  REQUIRE(crystalStructures->getDataStoreRef().getDataFormat() == "HDF5-OOC");
}
} // namespace nx::core::UnitTest::FeatureNeighbors
