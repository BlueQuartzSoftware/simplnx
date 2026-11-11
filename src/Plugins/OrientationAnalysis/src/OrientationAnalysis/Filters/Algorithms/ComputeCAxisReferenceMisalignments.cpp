#include "ComputeCAxisReferenceMisalignments.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 4096;
constexpr std::string_view k_ProgressLabel = "Computing C-Axis Reference Misalignments";
} // namespace

// -----------------------------------------------------------------------------
ComputeCAxisReferenceMisalignments::ComputeCAxisReferenceMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeCAxisReferenceMisalignmentsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(messageHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
Result<> ComputeCAxisReferenceMisalignments::operator()()
{
  const auto& quatsStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  const usize total = quatsStoreRef.getNumberOfTuples();
  auto& outputStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputArrayPath).getDataStoreRef();

  const auto& referenceValuesRef = m_InputValues->ReferenceDirection;
  Eigen::Vector3d referenceDirection(referenceValuesRef[0], referenceValuesRef[1], referenceValuesRef[2]);
  referenceDirection.normalize();

  Result<> result;
  auto ioSucceeded = [&result](Result<> ioResult) {
    result = MergeResults(std::move(result), std::move(ioResult));
    return result.valid();
  };

  // Cache only the small ensemble array. All tuple-level buffers have a fixed bound.
  const auto& crystalStructuresStoreRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();
  std::vector<uint32> crystalStructures(crystalStructuresStoreRef.getNumberOfTuples());
  if(!ioSucceeded(crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()))))
  {
    return result;
  }

  const auto& phasesStoreRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->PhasesArrayPath).getDataStoreRef();

  // Preflight guarantees that an enabled mask is exactly one of these types.
  const BoolArray* boolMaskPtr = nullptr;
  const UInt8Array* byteMaskPtr = nullptr;
  if(m_InputValues->UseMask)
  {
    boolMaskPtr = m_DataStructure.getDataAs<BoolArray>(m_InputValues->MaskArrayPath);
    if(boolMaskPtr == nullptr)
    {
      byteMaskPtr = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->MaskArrayPath);
    }
  }

  std::vector<float32> quats(k_ChunkTuples * 4);
  std::vector<int32> phases(k_ChunkTuples);
  std::vector<float32> output(k_ChunkTuples);
  std::vector<uint8> byteMaskBuffer(byteMaskPtr != nullptr ? k_ChunkTuples : 0);
  // Project-rule exception: std::vector<bool> is bit-packed and cannot provide the contiguous bool* that bulk copyIntoBuffer needs.
  std::unique_ptr<bool[]> boolMaskBufferPtr = boolMaskPtr != nullptr ? std::make_unique<bool[]>(k_ChunkTuples) : nullptr;

  usize nonHexagonalCount = 0;
  ThrottledMessageHandler progress(m_MessageHandler);
  for(usize offset = 0; offset < total; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return result;
    }
    const usize count = std::min(k_ChunkTuples, total - offset);
    if(!ioSucceeded(quatsStoreRef.copyIntoBuffer(offset * 4, nonstd::span<float32>(quats.data(), count * 4))) ||
       !ioSucceeded(phasesStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(phases.data(), count))))
    {
      return result;
    }
    if(boolMaskPtr != nullptr && !ioSucceeded(boolMaskPtr->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(boolMaskBufferPtr.get(), count))))
    {
      return result;
    }
    if(byteMaskPtr != nullptr && !ioSucceeded(byteMaskPtr->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<uint8>(byteMaskBuffer.data(), count))))
    {
      return result;
    }

    for(usize tupleIdx = 0; tupleIdx < count; ++tupleIdx)
    {
      output[tupleIdx] = std::numeric_limits<float32>::quiet_NaN();
      if((boolMaskPtr != nullptr && !boolMaskBufferPtr[tupleIdx]) || (byteMaskPtr != nullptr && byteMaskBuffer[tupleIdx] == 0))
      {
        continue;
      }
      const int32 phase = phases[tupleIdx];
      if(phase < 0 || static_cast<usize>(phase) >= crystalStructures.size())
      {
        return MergeResults(std::move(result),
                            MakeErrorResult(k_PhaseOutOfRangeError, fmt::format("Phases array '{}' has value {} at tuple index {}, but Crystal Structures array '{}' has {} tuples. "
                                                                                "Valid phase indices are in [0, {}). Correct this phase value or select the matching Crystal Structures array.",
                                                                                m_InputValues->PhasesArrayPath.toString(), phase, offset + tupleIdx,
                                                                                m_InputValues->CrystalStructuresArrayPath.toString(), crystalStructures.size(), crystalStructures.size())));
      }
      const uint32 crystalStructure = crystalStructures[phase];
      if(crystalStructure != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructure != ebsdlib::CrystalStructure::Hexagonal_Low)
      {
        if(crystalStructure != ebsdlib::CrystalStructure::UnknownCrystalStructure)
        {
          ++nonHexagonalCount;
        }
        continue;
      }
      const usize quatIdx = tupleIdx * 4;
      ebsdlib::QuaternionDType quat(quats[quatIdx], quats[quatIdx + 1], quats[quatIdx + 2], quats[quatIdx + 3]);
      const float64 quatLength = quat.length();
      if(!std::isfinite(quatLength) || quatLength == 0.0)
      {
        continue; // Output stays NaN: this quaternion is not a rotation.
      }
      quat = quat.normalize().getPositiveOrientation();
      const ebsdlib::OrientationMatrixDType oMatrix = quat.toOrientationMatrix();
      // The transpose maps crystal [001] into the sample frame. Opposite c-axes represent the same axis.
      Eigen::Vector3d cAxis = oMatrix.transpose() * Eigen::Vector3d(0.0, 0.0, 1.0);
      cAxis.normalize();
      const float64 cosine = std::clamp(std::abs(cAxis.dot(referenceDirection)), 0.0, 1.0);
      output[tupleIdx] = static_cast<float32>(std::acos(cosine) * (180.0 / numbers::pi));
    }
    if(!ioSucceeded(outputStoreRef.copyFromBuffer(offset, nonstd::span<const float32>(output.data(), count))))
    {
      return result;
    }
    progress.updateCount(k_ProgressLabel, offset + count, total);
  }
  progress.flush();

  if(nonHexagonalCount != 0)
  {
    result.warnings().push_back({k_NonHexagonalPhasesWarning, fmt::format("{} tuples in '{}' have non-hexagonal phases and received NaN. Only Hexagonal_High (6/mmm) and Hexagonal_Low (6/m) "
                                                                          "phases are supported. To suppress this warning, enable Use Mask Array with a mask that excludes the non-hexagonal tuples.",
                                                                          nonHexagonalCount, m_InputValues->QuatsArrayPath.toString())});
  }
  return result;
}
