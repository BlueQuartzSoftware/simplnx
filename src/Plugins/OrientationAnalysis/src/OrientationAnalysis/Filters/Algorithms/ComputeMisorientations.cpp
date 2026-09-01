#include "ComputeMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/OrientationFwd.hpp>

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
/// Maximum tuple count per transfer bounds local memory.
constexpr usize k_MaxChunkTuples = 65536;
constexpr usize k_EulerComponents = 3;
constexpr usize k_OutputComponents = 4;

usize CalculateChunkTuples(const AbstractDataStore<float32>& inputOrientationsRef)
{
  const usize totalTuples = inputOrientationsRef.getNumberOfTuples();
  const ShapeType& tupleShape = inputOrientationsRef.getTupleShape();
  if(totalTuples == 0 || tupleShape.size() <= 1 || tupleShape.front() == 0)
  {
    return std::max<usize>(1, std::min(k_MaxChunkTuples, totalTuples));
  }

  // Whole slabs use the OOC rectangular-hyperslab path without unbounded scratch.
  const usize slabTuples = totalTuples / tupleShape.front();
  if(slabTuples == 0 || slabTuples > k_MaxChunkTuples)
  {
    return k_MaxChunkTuples;
  }
  const usize slabsPerChunk = std::max<usize>(1, k_MaxChunkTuples / slabTuples);
  return std::min(totalTuples, slabTuples * slabsPerChunk);
}

void ComputeMisorientation(const ebsdlib::QuatD& q1, const ebsdlib::QuatD& q2, float32* outputMisorientations, usize laueClass, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps,
                           usize tupleIdx)
{
  const ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass]->calculateMisorientation(q1, q2);

  const usize outputOffset = tupleIdx * k_OutputComponents;
  outputMisorientations[outputOffset] = axisAngle[0];
  outputMisorientations[outputOffset + 1] = axisAngle[1];
  outputMisorientations[outputOffset + 2] = axisAngle[2];
  outputMisorientations[outputOffset + 3] = axisAngle[3] * nx::core::Constants::k_180OverPiD;
}

/**
 * @class ArraysSecondOrientationProvider
 * @brief Provides a second orientation from local Euler buffers.
 */
class ArraysSecondOrientationProvider
{
public:
  ArraysSecondOrientationProvider(const AbstractDataStore<float32>& inputOrientationsRef, usize chunkTuples)
  : m_InputOrientationsRef(inputOrientationsRef)
  , m_EulersBuffer(std::make_unique<float32[]>(chunkTuples * k_EulerComponents))
  {
  }

  Result<> prepareChunk(usize tupleOffset, usize tupleCount)
  {
    return m_InputOrientationsRef.copyIntoBuffer(tupleOffset * k_EulerComponents, nonstd::span<float32>(m_EulersBuffer.get(), tupleCount * k_EulerComponents));
  }

  void computeMisorientation(const ebsdlib::QuatD& q1, usize laueClass, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, float32* outputMisorientations, usize tupleIdx) const
  {
    const usize eulerOffset = tupleIdx * k_EulerComponents;
    const ebsdlib::QuatD q2 = ebsdlib::EulerDType(m_EulersBuffer[eulerOffset], m_EulersBuffer[eulerOffset + 1], m_EulersBuffer[eulerOffset + 2]).toQuaternion();
    ComputeMisorientation(q1, q2, outputMisorientations, laueClass, orientationOps, tupleIdx);
  }

private:
  const AbstractDataStore<float32>& m_InputOrientationsRef;
  std::unique_ptr<float32[]> m_EulersBuffer;
};

/**
 * @class ReferenceSecondOrientationProvider
 * @brief Provides one fixed second orientation.
 */
class ReferenceSecondOrientationProvider
{
public:
  explicit ReferenceSecondOrientationProvider(const ebsdlib::QuatD& referenceOrientation)
  : m_ReferenceOrientation(referenceOrientation)
  {
  }

  Result<> prepareChunk(usize, usize) const
  {
    return {};
  }

  void computeMisorientation(const ebsdlib::QuatD& q1, usize laueClass, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, float32* outputMisorientations, usize tupleIdx) const
  {
    ComputeMisorientation(q1, m_ReferenceOrientation, outputMisorientations, laueClass, orientationOps, tupleIdx);
  }

private:
  ebsdlib::QuatD m_ReferenceOrientation;
};

/**
 * @brief Computes misorientations through bounded chunks.
 * @tparam SecondOrientationProvider Provides the second orientation.
 * @param dataStructure Provides selected arrays.
 * @param inputValues Identifies selected arrays and mode.
 * @param secondOrientationProvider Provides second orientations.
 * @param chunkTuples Specifies tuples per transfer.
 * @param shouldCancel Signals cancellation.
 * @return Success, or a bulk-I/O error.
 */
template <typename SecondOrientationProvider>
Result<> ComputeMisorientationChunks(DataStructure& dataStructure, const ComputeMisorientationsInputValues& inputValues, SecondOrientationProvider& secondOrientationProvider, usize chunkTuples,
                                     const std::atomic_bool& shouldCancel)
{
  const auto& inputOrientationsRef = dataStructure.getDataRefAs<Float32Array>(inputValues.InputOrientationPath1).getDataStoreRef();
  const auto& cellPhasesRef = dataStructure.getDataRefAs<Int32Array>(inputValues.InputPhasesArrayPath).getDataStoreRef();
  const auto& crystalStructuresStoreRef = dataStructure.getDataRefAs<UInt32Array>(inputValues.InputCrystalStructuresArrayPath).getDataStoreRef();
  auto& outputMisorientationsRef = dataStructure.getDataRefAs<Float32Array>(inputValues.OutputMisorientationsPath).getDataStoreRef();

  if(shouldCancel)
  {
    return {};
  }

  const usize numCrystalStructures = crystalStructuresStoreRef.getNumberOfTuples();
  std::vector<uint32> crystalStructures(numCrystalStructures);
  Result<> result = crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()));
  if(result.invalid())
  {
    return result;
  }

  const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  const usize totalPoints = inputOrientationsRef.getNumberOfTuples();
  if(totalPoints == 0)
  {
    return {};
  }
  auto inputOrientationsBuffer = std::make_unique<float32[]>(chunkTuples * k_EulerComponents);
  auto cellPhasesBuffer = std::make_unique<int32[]>(chunkTuples);
  auto outputMisorientationsBuffer = std::make_unique<float32[]>(chunkTuples * k_OutputComponents);

  for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += chunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize tupleCount = std::min(chunkTuples, totalPoints - tupleOffset);
    result = inputOrientationsRef.copyIntoBuffer(tupleOffset * k_EulerComponents, nonstd::span<float32>(inputOrientationsBuffer.get(), tupleCount * k_EulerComponents));
    if(result.invalid())
    {
      return result;
    }
    result = cellPhasesRef.copyIntoBuffer(tupleOffset, nonstd::span<int32>(cellPhasesBuffer.get(), tupleCount));
    if(result.invalid())
    {
      return result;
    }
    result = secondOrientationProvider.prepareChunk(tupleOffset, tupleCount);
    if(result.invalid())
    {
      return result;
    }

    for(usize tupleIdx = 0; tupleIdx < tupleCount; tupleIdx++)
    {
      const usize outputOffset = tupleIdx * k_OutputComponents;
      const int32 phase = cellPhasesBuffer[tupleIdx];
      if(phase > 0)
      {
        const usize eulerOffset = tupleIdx * k_EulerComponents;
        const ebsdlib::QuatD q1 = ebsdlib::EulerDType(inputOrientationsBuffer[eulerOffset], inputOrientationsBuffer[eulerOffset + 1], inputOrientationsBuffer[eulerOffset + 2]).toQuaternion();
        const usize laueClass = static_cast<usize>(crystalStructures[static_cast<usize>(phase)]);
        secondOrientationProvider.computeMisorientation(q1, laueClass, orientationOps, outputMisorientationsBuffer.get(), tupleIdx);
      }
      else
      {
        outputMisorientationsBuffer[outputOffset] = 0.0F;
        outputMisorientationsBuffer[outputOffset + 1] = 0.0F;
        outputMisorientationsBuffer[outputOffset + 2] = 0.0F;
        outputMisorientationsBuffer[outputOffset + 3] = 0.0F;
      }
    }

    result = outputMisorientationsRef.copyFromBuffer(tupleOffset * k_OutputComponents, nonstd::span<const float32>(outputMisorientationsBuffer.get(), tupleCount * k_OutputComponents));
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}
} // namespace

ComputeMisorientations::ComputeMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                               ComputeMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

ComputeMisorientations::~ComputeMisorientations() noexcept = default;

Result<> ComputeMisorientations::operator()()
{
  if(m_InputValues->ComputationType == compute_misorientations_constants::k_UseArraysIndex)
  {
    const auto& inputOrientations1Ref = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputOrientationPath1).getDataStoreRef();
    const auto& inputOrientations2Ref = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputOrientationPath2).getDataStoreRef();
    const usize chunkTuples = CalculateChunkTuples(inputOrientations1Ref);
    ArraysSecondOrientationProvider secondOrientationProvider(inputOrientations2Ref, chunkTuples);
    return ComputeMisorientationChunks(m_DataStructure, *m_InputValues, secondOrientationProvider, chunkTuples, m_ShouldCancel);
  }

  if(m_InputValues->ComputationType == compute_misorientations_constants::k_UseReferenceAxesIndex)
  {
    const auto& inputOrientations1Ref = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputOrientationPath1).getDataStoreRef();
    const usize chunkTuples = CalculateChunkTuples(inputOrientations1Ref);
    Eigen::Vector3d axis(m_InputValues->ReferenceOrientation[0], m_InputValues->ReferenceOrientation[1], m_InputValues->ReferenceOrientation[2]);
    axis.normalize();
    const ebsdlib::AxisAngleDType referenceOrientation(axis[0], axis[1], axis[2], m_InputValues->ReferenceOrientation[3] * nx::core::Constants::k_PiOver180D);
    const ebsdlib::QuatD referenceQuaternion = referenceOrientation.toQuaternion();
    ReferenceSecondOrientationProvider secondOrientationProvider(referenceQuaternion);
    return ComputeMisorientationChunks(m_DataStructure, *m_InputValues, secondOrientationProvider, chunkTuples, m_ShouldCancel);
  }

  return {};
}
