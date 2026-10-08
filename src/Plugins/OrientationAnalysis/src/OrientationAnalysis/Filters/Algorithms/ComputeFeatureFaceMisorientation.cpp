#include "ComputeFeatureFaceMisorientation.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <vector>

using LaueOpsShPtrType = std::shared_ptr<ebsdlib::LaueOps>;
using LaueOpsContainer = std::vector<LaueOpsShPtrType>;

using namespace nx::core;

namespace
{
constexpr usize k_ValidationChunkTuples = 65536;

/**
 * @brief Validates matching positive Feature Phases referenced by each face.
 * @param faceLabelsArrayRef Identifies the two Features adjacent to each face.
 * @param featurePhasesArrayRef Maps Feature indices to Phase indices.
 * @param crystalStructuresArrayRef Defines the valid Phase-index range.
 * @param inputValues Provides DataPaths for error diagnostics.
 * @return Success, or an error for invalid Phase indices or bulk I/O.
 */
Result<> ValidateParticipatingFeaturePhases(const Int32Array& faceLabelsArrayRef, const Int32Array& featurePhasesArrayRef, const UInt32Array& crystalStructuresArrayRef,
                                            const ComputeFeatureFaceMisorientationInputValues& inputValues)
{
  const usize numFeatures = featurePhasesArrayRef.getNumberOfTuples();
  std::vector<int32> featurePhasesCache(numFeatures);
  if(Result<> readResult = featurePhasesArrayRef.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(featurePhasesCache.data(), featurePhasesCache.size())); readResult.invalid())
  {
    return readResult;
  }

  const usize numCrystalStructures = crystalStructuresArrayRef.getNumberOfTuples();
  const auto& faceLabelsStoreRef = faceLabelsArrayRef.getDataStoreRef();
  const usize numFaces = faceLabelsArrayRef.getNumberOfTuples();
  std::vector<int32> faceLabelsBuffer(k_ValidationChunkTuples * 2);
  for(usize faceOffset = 0; faceOffset < numFaces; faceOffset += k_ValidationChunkTuples)
  {
    const usize faceCount = std::min(k_ValidationChunkTuples, numFaces - faceOffset);
    if(Result<> readResult = faceLabelsStoreRef.copyIntoBuffer(faceOffset * 2, nonstd::span<int32>(faceLabelsBuffer.data(), faceCount * 2)); readResult.invalid())
    {
      return readResult;
    }

    for(usize chunkFaceIdx = 0; chunkFaceIdx < faceCount; chunkFaceIdx++)
    {
      const int32 frontFeatureIdx = faceLabelsBuffer[chunkFaceIdx * 2];
      const int32 backFeatureIdx = faceLabelsBuffer[chunkFaceIdx * 2 + 1];
      const int32 frontPhaseIdx = frontFeatureIdx > 0 ? featurePhasesCache[frontFeatureIdx] : 0;
      const int32 backPhaseIdx = backFeatureIdx > 0 ? featurePhasesCache[backFeatureIdx] : 0;
      if(frontPhaseIdx > 0 && frontPhaseIdx == backPhaseIdx && static_cast<usize>(frontPhaseIdx) >= numCrystalStructures)
      {
        return MakeErrorResult(
            -98412,
            fmt::format(
                "Feature Phases array '{}' has value {} at Feature index {}, referenced by face {}, but Crystal Structures array '{}' contains {} tuples. Valid positive Phase indices are in [1, {}).",
                inputValues.featurePhasesArrayPath.toString(), frontPhaseIdx, frontFeatureIdx, faceOffset + chunkFaceIdx, inputValues.crystalStructuresArrayPath.toString(), numCrystalStructures,
                numCrystalStructures));
      }
    }
  }
  return {};
}
} // namespace

/**
 * @class ComputeFeatureMisorientationPerTriangleImpl
 * @brief Calculates feature misorientation for a contiguous range of surface-mesh faces.
 */
class ComputeFeatureMisorientationPerTriangleImpl
{
  const Int32Array& m_FaceLabels;
  const Int32Array& m_FeaturePhases;
  const Float32Array& m_FeatureAvgQuats;
  const UInt32Array& m_CrystalStructures;
  const std::atomic_bool& m_ShouldCancel;
  Float32Array& m_Misorientations;
  LaueOpsContainer m_LaueOrientationOps;

public:
  ComputeFeatureMisorientationPerTriangleImpl(const Int32Array& labels, const Int32Array& phases, const Float32Array& quats, const UInt32Array& crystalStructures, const std::atomic_bool& shouldCancel,
                                              Float32Array& output)
  : m_FaceLabels(labels)
  , m_FeaturePhases(phases)
  , m_FeatureAvgQuats(quats)
  , m_CrystalStructures(crystalStructures)
  , m_ShouldCancel(shouldCancel)
  , m_Misorientations(output)
  {
    m_LaueOrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  }
  virtual ~ComputeFeatureMisorientationPerTriangleImpl() = default;

  void generate(const usize start, const usize end) const
  {
    // Since our meshes use unified triangles, there are two triangles
    // per entry. These are distinguished via the face labels array,
    // which contains the feature id of each respective face. Here, the
    // first entry in face labels is denoted as "front" and the second "back"
    int32 frontFeatureIdx = 0;
    int32 backFeatureIdx = 0;
    int32 frontPhaseIdx = 0;
    int32 backPhaseIdx = 0;

    for(usize triangleIdx = start; triangleIdx < end; triangleIdx++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      frontFeatureIdx = m_FaceLabels[2 * triangleIdx];
      backFeatureIdx = m_FaceLabels[2 * triangleIdx + 1];
      if(frontFeatureIdx > 0)
      {
        frontPhaseIdx = m_FeaturePhases[frontFeatureIdx];
      }
      else
      {
        frontPhaseIdx = 0;
      }
      if(backFeatureIdx > 0)
      {
        backPhaseIdx = m_FeaturePhases[backFeatureIdx];
      }
      else
      {
        backPhaseIdx = 0;
      }
      if(frontPhaseIdx > 0 && frontPhaseIdx == backPhaseIdx)
      {
        const uint32 currentLaueIndex = m_CrystalStructures[frontPhaseIdx];
        if(currentLaueIndex < m_LaueOrientationOps.size())
        {
          float32 quat0 = m_FeatureAvgQuats[frontFeatureIdx * 4];
          float32 quat1 = m_FeatureAvgQuats[frontFeatureIdx * 4 + 1];
          float32 quat2 = m_FeatureAvgQuats[frontFeatureIdx * 4 + 2];
          float32 quat3 = m_FeatureAvgQuats[frontFeatureIdx * 4 + 3];
          ebsdlib::QuatD q1(quat0, quat1, quat2, quat3);
          quat0 = m_FeatureAvgQuats[backFeatureIdx * 4];
          quat1 = m_FeatureAvgQuats[backFeatureIdx * 4 + 1];
          quat2 = m_FeatureAvgQuats[backFeatureIdx * 4 + 2];
          quat3 = m_FeatureAvgQuats[backFeatureIdx * 4 + 3];
          ebsdlib::QuatD q2(quat0, quat1, quat2, quat3);
          ebsdlib::AxisAngleDType axisAngle = m_LaueOrientationOps[currentLaueIndex]->calculateMisorientation(q1, q2);
          m_Misorientations.setValue(triangleIdx, static_cast<float32>(axisAngle[3] * Constants::k_180OverPiD));
          continue;
        }
      }
      m_Misorientations.setValue(triangleIdx, static_cast<float>(std::nan("0")));
    }
  }

  /**
   * @brief Processes a face range through the parallel-algorithm interface.
   * @param range Identifies the half-open face range.
   */
  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }
};

namespace
{
Result<> ComputeFeatureMisorientationInBlocks(const Int32Array& faceLabels, const Int32Array& phases, const Float32Array& quats, const UInt32Array& structures, Float32Array& misorientations,
                                              const std::atomic_bool& shouldCancel)
{
  const usize numTriangles = faceLabels.getNumberOfTuples();
  if(shouldCancel || numTriangles == 0)
  {
    return {};
  }
  Result<> result;
  // Cache feature and ensemble lookup tables once; stream the per-face arrays.
  std::vector<int32> featurePhases(phases.getSize());
  result = MergeResults(std::move(result), phases.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(featurePhases.data(), featurePhases.size())));
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(-98420, "Error reading feature phases for face misorientation."));
  }
  std::vector<float32> featureAvgQuats(quats.getSize());
  result = MergeResults(std::move(result), quats.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(featureAvgQuats.data(), featureAvgQuats.size())));
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(-98421, "Error reading feature average quaternions for face misorientation."));
  }
  std::vector<uint32> crystalStructures(structures.getSize());
  result = MergeResults(std::move(result), structures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size())));
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(-98422, "Error reading crystal structures for face misorientation."));
  }

  const LaueOpsContainer orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  // Two labels and one scalar result per triangle use at most 3 MiB.
  constexpr usize k_BlockTriangles = 262144;
  const usize bufferTriangles = std::min(k_BlockTriangles, numTriangles);
  std::vector<int32> faceLabelsBuffer(bufferTriangles * 2);
  std::vector<float32> misorientationsBuffer(bufferTriangles);
  for(usize start = 0; start < numTriangles;)
  {
    if(shouldCancel)
    {
      return result;
    }
    const usize count = std::min(k_BlockTriangles, numTriangles - start);
    result = MergeResults(std::move(result), faceLabels.getDataStoreRef().copyIntoBuffer(start * 2, nonstd::span<int32>(faceLabelsBuffer.data(), count * 2)));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-98423, fmt::format("Error reading face labels for face misorientation at triangle {} ({} triangles).", start, count)));
    }

    // Keep the in-core quaternion construction, Laue operation and float conversion.
    int32 frontFeatureIdx = 0;
    int32 backFeatureIdx = 0;
    int32 frontPhaseIdx = 0;
    int32 backPhaseIdx = 0;

    for(usize triangleIdx = 0; triangleIdx < count; triangleIdx++)
    {
      frontFeatureIdx = faceLabelsBuffer[2 * triangleIdx];
      backFeatureIdx = faceLabelsBuffer[2 * triangleIdx + 1];
      if(frontFeatureIdx > 0)
      {
        frontPhaseIdx = featurePhases[frontFeatureIdx];
      }
      else
      {
        frontPhaseIdx = 0;
      }
      if(backFeatureIdx > 0)
      {
        backPhaseIdx = featurePhases[backFeatureIdx];
      }
      else
      {
        backPhaseIdx = 0;
      }
      if(frontPhaseIdx > 0 && frontPhaseIdx == backPhaseIdx)
      {
        const uint32 currentLaueIndex = crystalStructures[frontPhaseIdx];
        if(currentLaueIndex < orientationOps.size())
        {
          float32 quat0 = featureAvgQuats[frontFeatureIdx * 4];
          float32 quat1 = featureAvgQuats[frontFeatureIdx * 4 + 1];
          float32 quat2 = featureAvgQuats[frontFeatureIdx * 4 + 2];
          float32 quat3 = featureAvgQuats[frontFeatureIdx * 4 + 3];
          ebsdlib::QuatD q1(quat0, quat1, quat2, quat3);
          quat0 = featureAvgQuats[backFeatureIdx * 4];
          quat1 = featureAvgQuats[backFeatureIdx * 4 + 1];
          quat2 = featureAvgQuats[backFeatureIdx * 4 + 2];
          quat3 = featureAvgQuats[backFeatureIdx * 4 + 3];
          ebsdlib::QuatD q2(quat0, quat1, quat2, quat3);
          ebsdlib::AxisAngleDType axisAngle = orientationOps[currentLaueIndex]->calculateMisorientation(q1, q2);
          misorientationsBuffer[triangleIdx] = static_cast<float32>(axisAngle[3] * Constants::k_180OverPiD);
          continue;
        }
      }
      misorientationsBuffer[triangleIdx] = static_cast<float>(std::nan("0"));
    }
    result = MergeResults(std::move(result), misorientations.getDataStoreRef().copyFromBuffer(start, nonstd::span<const float32>(misorientationsBuffer.data(), count)));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-98424, fmt::format("Error writing face misorientations at triangle {} ({} triangles).", start, count)));
    }
    start += count;
  }
  return result;
}
} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureFaceMisorientation::ComputeFeatureFaceMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   ComputeFeatureFaceMisorientationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureFaceMisorientation::~ComputeFeatureFaceMisorientation() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureFaceMisorientation::operator()()
{
  const auto& faceLabelsArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->surfaceMeshFaceLabelsArrayPath);
  const auto& avgQuatsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgQuatsArrayPath);
  const auto& featurePhasesArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->featurePhasesArrayPath);
  const auto& crystalStructuresArrayRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
  auto& misorientationsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->misorientationArrayPath);
  const usize numTriangles = faceLabelsArrayRef.getNumberOfTuples();

  if(Result<> validationResult = ValidateParticipatingFeaturePhases(faceLabelsArrayRef, featurePhasesArrayRef, crystalStructuresArrayRef, *m_InputValues); validationResult.invalid())
  {
    return validationResult;
  }

  const bool usesOutOfCoreStore = AnyOutOfCore({&faceLabelsArrayRef, &avgQuatsArrayRef, &featurePhasesArrayRef, &crystalStructuresArrayRef, &misorientationsArrayRef});
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  if(useOutOfCoreAlgorithm)
  {
    return ComputeFeatureMisorientationInBlocks(faceLabelsArrayRef, featurePhasesArrayRef, avgQuatsArrayRef, crystalStructuresArrayRef, misorientationsArrayRef, m_ShouldCancel);
  }

  ParallelDataAlgorithm parallelTask;
  parallelTask.setRange(0, numTriangles);
  parallelTask.setParallelizationEnabled(false);
  parallelTask.execute(ComputeFeatureMisorientationPerTriangleImpl(faceLabelsArrayRef, featurePhasesArrayRef, avgQuatsArrayRef, crystalStructuresArrayRef, m_ShouldCancel, misorientationsArrayRef));
  return {};
}
