#include "ComputeCAxisReferenceMisalignments.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string_view>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 4096;
constexpr std::string_view k_ProgressLabel = "Computing C-Axis Reference Misalignments";
} // namespace

ComputeCAxisReferenceMisalignments::ComputeCAxisReferenceMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeCAxisReferenceMisalignmentsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(messageHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

Result<> ComputeCAxisReferenceMisalignments::ValidateReferenceDirection(const std::vector<float32>& referenceDirection)
{
  if(referenceDirection.size() != 3)
  {
    return MakeErrorResult(-98710, fmt::format("Reference Direction has {} components. Enter exactly three sample-frame components [X, Y, Z].", referenceDirection.size()));
  }
  const float64 norm = std::hypot(static_cast<float64>(referenceDirection[0]), static_cast<float64>(referenceDirection[1]), static_cast<float64>(referenceDirection[2]));
  if(!std::isfinite(referenceDirection[0]) || !std::isfinite(referenceDirection[1]) || !std::isfinite(referenceDirection[2]) || norm <= k_MinReferenceDirectionNorm)
  {
    return MakeErrorResult(-98710,
                           fmt::format("Reference Direction [{}, {}, {}] must contain finite components and have length greater than {}. Enter a nonzero sample-frame direction, such as [0, 0, 1].",
                                       referenceDirection[0], referenceDirection[1], referenceDirection[2], k_MinReferenceDirectionNorm));
  }
  return {};
}

Result<> ComputeCAxisReferenceMisalignments::operator()()
{
  if(auto validation = ValidateReferenceDirection(m_InputValues->ReferenceDirection); validation.invalid())
  {
    return validation;
  }
  const auto& reference = m_InputValues->ReferenceDirection;
  Eigen::Vector3d referenceDirection(reference[0], reference[1], reference[2]);
  referenceDirection.normalize();

  Result<> result;
  auto ioSucceeded = [&result](Result<> ioResult) {
    result = MergeResults(std::move(result), std::move(ioResult));
    return result.valid();
  };

  // Cache only the small ensemble array. All tuple-level buffers have a fixed bound.
  const auto& crystalStructuresStore = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();
  std::vector<uint32> crystalStructures(crystalStructuresStore.getNumberOfTuples());
  if(!ioSucceeded(crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()))))
  {
    return result;
  }

  const auto& quatsStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  const auto& phasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->PhasesArrayPath).getDataStoreRef();
  auto& outputStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputArrayPath).getDataStoreRef();
  const usize total = quatsStore.getNumberOfTuples();

  const BoolArray* boolMask = nullptr;
  const UInt8Array* byteMask = nullptr;
  if(m_InputValues->UseMask)
  {
    const auto& mask = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    if(mask.getDataType() == DataType::boolean)
    {
      boolMask = &dynamic_cast<const BoolArray&>(mask);
    }
    else
    {
      byteMask = &dynamic_cast<const UInt8Array&>(mask);
    }
  }

  std::vector<float32> quats(k_ChunkTuples * 4);
  std::vector<int32> phases(k_ChunkTuples);
  std::vector<float32> output(k_ChunkTuples);
  std::vector<uint8> byteMaskBuffer(byteMask != nullptr ? k_ChunkTuples : 0);
  // Project-rule exception: std::vector<bool> is bit-packed and cannot provide the contiguous bool* that bulk copyIntoBuffer needs.
  std::unique_ptr<bool[]> boolMaskBuffer = boolMask != nullptr ? std::make_unique<bool[]>(k_ChunkTuples) : nullptr;

  usize nonHexagonalCount = 0;
  ThrottledMessageHandler progress(m_MessageHandler);
  for(usize offset = 0; offset < total; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return result;
    }
    const usize count = std::min(k_ChunkTuples, total - offset);
    if(!ioSucceeded(quatsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(quats.data(), count * 4))) ||
       !ioSucceeded(phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phases.data(), count))))
    {
      return result;
    }
    if(boolMask != nullptr && !ioSucceeded(boolMask->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(boolMaskBuffer.get(), count))))
    {
      return result;
    }
    if(byteMask != nullptr && !ioSucceeded(byteMask->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<uint8>(byteMaskBuffer.data(), count))))
    {
      return result;
    }

    for(usize i = 0; i < count; ++i)
    {
      output[i] = std::numeric_limits<float32>::quiet_NaN();
      if((boolMask != nullptr && !boolMaskBuffer[i]) || (byteMask != nullptr && byteMaskBuffer[i] == 0))
      {
        continue;
      }
      const int32 phase = phases[i];
      if(phase < 0 || static_cast<usize>(phase) >= crystalStructures.size())
      {
        return MergeResults(std::move(result), MakeErrorResult(-98712, fmt::format("Phases array '{}' has value {} at tuple index {}, but Crystal Structures array '{}' has {} tuples. "
                                                                                   "Valid phase indices are in [0, {}). Correct this phase value or select the matching Crystal Structures array.",
                                                                                   m_InputValues->PhasesArrayPath.toString(), phase, offset + i, m_InputValues->CrystalStructuresArrayPath.toString(),
                                                                                   crystalStructures.size(), crystalStructures.size())));
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
      const usize qi = i * 4;
      const ebsdlib::OrientationMatrixDType oMatrix = ebsdlib::QuaternionDType(quats[qi], quats[qi + 1], quats[qi + 2], quats[qi + 3]).toOrientationMatrix();
      // The transpose maps crystal [001] into the sample frame. Opposite c-axes represent the same axis.
      Eigen::Vector3d cAxis = oMatrix.transpose() * Eigen::Vector3d(0.0, 0.0, 1.0);
      const float64 cAxisNorm = cAxis.norm();
      if(!std::isfinite(cAxisNorm) || cAxisNorm == 0.0)
      {
        continue; // An undefined orientation must not become a finite angle.
      }
      cAxis /= cAxisNorm;
      const float64 cosine = std::clamp(std::abs(cAxis.dot(referenceDirection)), 0.0, 1.0);
      output[i] = static_cast<float32>(std::acos(cosine) * (180.0 / numbers::pi));
    }
    if(!ioSucceeded(outputStore.copyFromBuffer(offset, nonstd::span<const float32>(output.data(), count))))
    {
      return result;
    }
    progress.updateCount(k_ProgressLabel, offset + count, total);
  }
  progress.flush();

  if(nonHexagonalCount != 0)
  {
    result.warnings().push_back({-98713, fmt::format("{} tuples in '{}' have non-hexagonal phases and received NaN. Only Hexagonal_High (6/mmm) and Hexagonal_Low (6/m) are supported. "
                                                     "Select hexagonal tuples with the mask if these phases should be excluded.",
                                                     nonHexagonalCount, m_InputValues->QuatsArrayPath.toString())});
  }
  return result;
}
