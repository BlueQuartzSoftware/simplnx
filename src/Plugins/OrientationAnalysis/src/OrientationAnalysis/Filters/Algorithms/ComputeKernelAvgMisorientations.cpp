#include "ComputeKernelAvgMisorientations.hpp"

#include "ComputeKernelAvgMisorientationsDirect.hpp"
#include "ComputeKernelAvgMisorientationsScanline.hpp"

#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <vector>

using namespace nx::core;

Result<> nx::core::ValidateKernelAvgMisorientationsPhaseIndices(DataStructure& dataStructure, const ComputeKernelAvgMisorientationsInputValues& inputValues)
{
  constexpr usize k_ValidationChunkTuples = 65536;
  const auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(inputValues.FeatureIdsArrayPath).getDataStoreRef();
  const auto& cellPhasesStoreRef = dataStructure.getDataRefAs<Int32Array>(inputValues.CellPhasesArrayPath).getDataStoreRef();
  const auto& crystalStructuresStoreRef = dataStructure.getDataRefAs<UInt32Array>(inputValues.CrystalStructuresArrayPath).getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStoreRef.getNumberOfTuples();
  std::vector<uint32> crystalStructuresCache(numCrystalStructures);
  if(Result<> readResult = crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.data(), crystalStructuresCache.size())); readResult.invalid())
  {
    return readResult;
  }
  const usize numLaueOperations = ebsdlib::LaueOps::GetAllOrientationOps().size();
  const usize totalTuples = featureIdsStoreRef.getNumberOfTuples();
  std::vector<int32> featureIdsBuffer(k_ValidationChunkTuples);
  std::vector<int32> cellPhasesBuffer(k_ValidationChunkTuples);
  for(usize tupleOffset = 0; tupleOffset < totalTuples; tupleOffset += k_ValidationChunkTuples)
  {
    const usize tupleCount = std::min(k_ValidationChunkTuples, totalTuples - tupleOffset);
    if(Result<> readResult = featureIdsStoreRef.copyIntoBuffer(tupleOffset, nonstd::span<int32>(featureIdsBuffer.data(), tupleCount)); readResult.invalid())
    {
      return readResult;
    }
    if(Result<> readResult = cellPhasesStoreRef.copyIntoBuffer(tupleOffset, nonstd::span<int32>(cellPhasesBuffer.data(), tupleCount)); readResult.invalid())
    {
      return readResult;
    }
    for(usize chunkTupleIdx = 0; chunkTupleIdx < tupleCount; chunkTupleIdx++)
    {
      const int32 currentPhaseIdx = cellPhasesBuffer[chunkTupleIdx];
      if(featureIdsBuffer[chunkTupleIdx] <= 0 || currentPhaseIdx <= 0)
      {
        continue;
      }
      if(static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
      {
        return MakeErrorResult(-67203,
                               fmt::format("Cell Phases array '{}' has value {} at voxel index {}, but Crystal Structures array '{}' contains {} tuples. Valid positive Phase indices are in [1, {}).",
                                           inputValues.CellPhasesArrayPath.toString(), currentPhaseIdx, tupleOffset + chunkTupleIdx, inputValues.CrystalStructuresArrayPath.toString(),
                                           numCrystalStructures, numCrystalStructures));
      }
      const uint32 currentLaueIndex = crystalStructuresCache[currentPhaseIdx];
      if(currentLaueIndex >= numLaueOperations)
      {
        return MakeErrorResult(-67204, fmt::format("Crystal Structures array '{}' has value {} at Phase index {}, but only {} Laue operations are available. Valid Laue indices are in [0, {}).",
                                                   inputValues.CrystalStructuresArrayPath.toString(), currentLaueIndex, currentPhaseIdx, numLaueOperations, numLaueOperations));
      }
    }
  }
  return {};
}

ComputeKernelAvgMisorientations::ComputeKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 ComputeKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeKernelAvgMisorientations::~ComputeKernelAvgMisorientations() noexcept = default;

Result<> ComputeKernelAvgMisorientations::operator()()
{
  const auto* featureIds = m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  const auto* phases = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CellPhasesArrayPath);
  const auto* quats = m_DataStructure.getDataAs<IDataArray>(m_InputValues->QuatsArrayPath);
  const auto* crystalStructures = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CrystalStructuresArrayPath);
  const auto* output = m_DataStructure.getDataAs<IDataArray>(m_InputValues->KernelAverageMisorientationsArrayName);

  return DispatchAlgorithm<ComputeKernelAvgMisorientationsDirect, ComputeKernelAvgMisorientationsScanline>({featureIds, phases, quats, crystalStructures, output}, m_DataStructure, m_MessageHandler,
                                                                                                           m_ShouldCancel, m_InputValues);
}
