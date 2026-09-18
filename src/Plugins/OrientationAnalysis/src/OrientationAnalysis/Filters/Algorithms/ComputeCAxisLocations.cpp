#include "ComputeCAxisLocations.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

ComputeCAxisLocations::ComputeCAxisLocations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeCAxisLocationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeCAxisLocations::~ComputeCAxisLocations() noexcept = default;

const std::atomic_bool& ComputeCAxisLocations::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeCAxisLocations::operator()()
{
  const auto& crystalStructuresArrayRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize phaseIdx = 1; phaseIdx < crystalStructuresArrayRef.size(); ++phaseIdx)
  {
    const auto crystalStructureType = crystalStructuresArrayRef[phaseIdx];
    const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
    allPhasesHexagonal = allPhasesHexagonal && isHex;
    noPhasesHexagonal = noPhasesHexagonal && !isHex;
  }

  if(noPhasesHexagonal)
  {
    return MakeErrorResult(-3522, "Finding the c-axis locations requires at least one phase to be Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back({-3523, "Finding the c-axis locations requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. All calculations for non Hexagonal phases will be "
                                        "skipped and a NaN value inserted."});
  }
  auto mergeWarnings = [&result](Result<> ioResult) { return MergeResults(std::move(result), std::move(ioResult)); };

  const auto& quatsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhasesArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& cAxisLocationsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CAxisLocationsArrayName);

  const usize totalPoints = quatsArrayRef.getNumberOfTuples();

  // The local ensemble cache avoids repeated cell-loop access.
  const usize numCrystalStructures = crystalStructuresArrayRef.getNumberOfTuples();
  std::vector<uint32> crystalStructuresCache(numCrystalStructures);
  if(Result<> ioResult = crystalStructuresArrayRef.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.data(), numCrystalStructures)); ioResult.invalid())
  {
    return mergeWarnings(std::move(ioResult));
  }

  constexpr usize k_ChunkSize = 65536;
  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3f c1{0.0f, 0.0f, 0.0f};

  auto& quatsStoreRef = quatsArrayRef.getDataStoreRef();
  auto& cellPhasesStoreRef = cellPhasesArrayRef.getDataStoreRef();
  auto& cAxisLocationsStoreRef = cAxisLocationsArrayRef.getDataStoreRef();

  for(usize chunkStart = 0; chunkStart < totalPoints; chunkStart += k_ChunkSize)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize chunkCount = std::min(k_ChunkSize, totalPoints - chunkStart);

    std::vector<float32> quatBuf(chunkCount * 4);
    std::vector<int32> cellPhasesChunk(chunkCount);
    std::vector<float32> outputBuf(chunkCount * 3);

    if(Result<> ioResult = quatsStoreRef.copyIntoBuffer(chunkStart * 4, nonstd::span<float32>(quatBuf.data(), chunkCount * 4)); ioResult.invalid())
    {
      return mergeWarnings(std::move(ioResult));
    }
    if(Result<> ioResult = cellPhasesStoreRef.copyIntoBuffer(chunkStart, nonstd::span<int32>(cellPhasesChunk.data(), chunkCount)); ioResult.invalid())
    {
      return mergeWarnings(std::move(ioResult));
    }

    for(usize chunkTupleIdx = 0; chunkTupleIdx < chunkCount; chunkTupleIdx++)
    {
      const int32 currentPhaseIdx = cellPhasesChunk[chunkTupleIdx];
      if(currentPhaseIdx < 0 || static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
      {
        return MakeErrorResult(-3524, fmt::format("Cell Phases array '{}' has value {} at voxel index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in [0, {}).",
                                                  m_InputValues->CellPhasesArrayPath.toString(), currentPhaseIdx, chunkStart + chunkTupleIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                                  numCrystalStructures, numCrystalStructures));
      }
      const auto crystalStructureType = crystalStructuresCache[currentPhaseIdx];
      if(crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low)
      {
        const usize qi = chunkTupleIdx * 4;
        ebsdlib::OrientationMatrixFType oMatrix = ebsdlib::QuaternionFType(quatBuf[qi], quatBuf[qi + 1], quatBuf[qi + 2], quatBuf[qi + 3]).toOrientationMatrix();
        c1 = oMatrix.transpose() * cAxis;
        c1.normalize();
        if(c1[2] < 0)
        {
          c1 *= -1.0f;
        }
        outputBuf[chunkTupleIdx * 3] = c1[0];
        outputBuf[chunkTupleIdx * 3 + 1] = c1[1];
        outputBuf[chunkTupleIdx * 3 + 2] = c1[2];
      }
      else
      {
        outputBuf[chunkTupleIdx * 3] = NAN;
        outputBuf[chunkTupleIdx * 3 + 1] = NAN;
        outputBuf[chunkTupleIdx * 3 + 2] = NAN;
      }
    }

    if(Result<> ioResult = cAxisLocationsStoreRef.copyFromBuffer(chunkStart * 3, nonstd::span<const float32>(outputBuf.data(), chunkCount * 3)); ioResult.invalid())
    {
      return mergeWarnings(std::move(ioResult));
    }
  }
  return result;
}
