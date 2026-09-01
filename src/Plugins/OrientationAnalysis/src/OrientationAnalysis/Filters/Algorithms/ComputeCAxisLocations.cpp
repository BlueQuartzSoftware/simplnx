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
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize i = 1; i < crystalStructures.size(); ++i)
  {
    const auto crystalStructureType = crystalStructures[i];
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

  const auto& quaternions = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& cAxisLocation = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CAxisLocationsArrayName);

  const usize totalPoints = quaternions.getNumberOfTuples();

  // The local ensemble cache avoids repeated cell-loop access.
  const usize numPhases = crystalStructures.getNumberOfTuples();
  std::vector<uint32> crystalStructuresBuf(numPhases);
  crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresBuf.data(), numPhases));

  constexpr usize k_ChunkSize = 65536;
  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3f c1{0.0f, 0.0f, 0.0f};

  auto& quatStore = quaternions.getDataStoreRef();
  auto& phaseStore = cellPhases.getDataStoreRef();
  auto& outputStore = cAxisLocation.getDataStoreRef();

  for(usize chunkStart = 0; chunkStart < totalPoints; chunkStart += k_ChunkSize)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize chunkCount = std::min(k_ChunkSize, totalPoints - chunkStart);

    std::vector<float32> quatBuf(chunkCount * 4);
    std::vector<int32> phaseBuf(chunkCount);
    std::vector<float32> outputBuf(chunkCount * 3);

    quatStore.copyIntoBuffer(chunkStart * 4, nonstd::span<float32>(quatBuf.data(), chunkCount * 4));
    phaseStore.copyIntoBuffer(chunkStart, nonstd::span<int32>(phaseBuf.data(), chunkCount));

    for(usize i = 0; i < chunkCount; i++)
    {
      const auto crystalStructureType = crystalStructuresBuf[phaseBuf[i]];
      if(crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low)
      {
        const usize qi = i * 4;
        ebsdlib::OrientationMatrixFType oMatrix = ebsdlib::QuaternionFType(quatBuf[qi], quatBuf[qi + 1], quatBuf[qi + 2], quatBuf[qi + 3]).toOrientationMatrix();
        c1 = oMatrix.transpose() * cAxis;
        c1.normalize();
        if(c1[2] < 0)
        {
          c1 *= -1.0f;
        }
        outputBuf[i * 3] = c1[0];
        outputBuf[i * 3 + 1] = c1[1];
        outputBuf[i * 3 + 2] = c1[2];
      }
      else
      {
        outputBuf[i * 3] = NAN;
        outputBuf[i * 3 + 1] = NAN;
        outputBuf[i * 3 + 2] = NAN;
      }
    }

    outputStore.copyFromBuffer(chunkStart * 3, nonstd::span<const float32>(outputBuf.data(), chunkCount * 3));
  }
  return result;
}
