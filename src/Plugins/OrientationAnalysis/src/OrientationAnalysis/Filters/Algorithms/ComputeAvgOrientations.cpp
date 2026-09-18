#include "ComputeAvgOrientations.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/DirectionalStats.hpp>
#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>

using namespace nx::core;

namespace
{

/**
 * @class VmfWatsonSamplingImpl
 * @brief Calculates resident vMF and Watson estimates.
 *
 * The direct path serializes this worker because it accesses shared DataArray
 * and DataStore objects. The scanline path groups records through checked bulk
 * I/O before it calls the same EbsdLib estimators.
 */
class VmfWatsonSamplingImpl
{
public:
  VmfWatsonSamplingImpl(ComputeAvgOrientations* filter, const ComputeAvgOrientationsInputValues* inputPtr, DataStructure& dataStruture, const std::vector<usize>& featureNumVoxels,
                        const std::map<int32, int32>& featureIdToPhaseMap)
  : m_Filter(filter)
  , m_InputValues(inputPtr)
  , m_DataStructure(dataStruture)
  , m_FeatureNumVoxels(featureNumVoxels)
  , m_FeatureIdToPhaseMap(featureIdToPhaseMap)
  {
  }

  virtual ~VmfWatsonSamplingImpl() = default;

  void operator()(const Range& range) const
  {
    // Cell arrays must share the same tuple count.
    Int32AbstractDataStore& featureIdsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath).getDataStoreRef();
    Int32AbstractDataStore& phasesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath).getDataStoreRef();
    Float32AbstractDataStore& quatsRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath).getDataStoreRef();
    UInt32AbstractDataStore& xtalRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath).getDataStoreRef();

    Float32AbstractDataStore* vmfQuatPtr = nullptr;
    Float32AbstractDataStore* vmfEulerPtr = nullptr;
    Float32AbstractDataStore* vmfKappaPtr = nullptr;
    if(m_InputValues->useVonMisesAverage)
    {
      vmfQuatPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFQuatsArrayPath).getDataStorePtr().lock().get();
      vmfEulerPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFEulerAnglesArrayPath).getDataStorePtr().lock().get();
      vmfKappaPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFKappaArrayPath).getDataStorePtr().lock().get();
    }

    Float32AbstractDataStore* watsonQuatPtr = nullptr;
    Float32AbstractDataStore* watsonEulerPtr = nullptr;
    Float32AbstractDataStore* watsonKappaPtr = nullptr;
    if(m_InputValues->useWatsonAverage)
    {
      watsonQuatPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonQuatsArrayPath).getDataStorePtr().lock().get();
      watsonEulerPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonEulerAnglesArrayPath).getDataStorePtr().lock().get();
      watsonKappaPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonKappaArrayPath).getDataStorePtr().lock().get();
    }

    const usize numVoxels = featureIdsRef.getNumberOfTuples();
    const usize numEnsembles = xtalRef.getNumberOfTuples();

    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();

    std::vector<ebsdlib::QuatD> fzQuats;
    for(usize featureId = range.min(); featureId < range.max(); featureId++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      if(m_FeatureNumVoxels[featureId] == 0)
      {
        continue;
      }

      const int32 phaseValue = m_FeatureIdToPhaseMap.at(static_cast<int32>(featureId));
      const uint32 laueClass = xtalRef[phaseValue];
      // Guard against an out-of-range crystal-structure enum (e.g. 999/Unknown); leave the
      // feature's outputs as the NaN that was filled before parallel execution.
      if(laueClass >= ops.size())
      {
        continue;
      }
      ebsdlib::LaueOps::Pointer op = ops[laueClass];

      fzQuats.clear();
      fzQuats.reserve(m_FeatureNumVoxels[featureId]);

      for(usize voxelIdx = 0; voxelIdx < numVoxels; voxelIdx++)
      {
        // Keep the gather gate identical to the counting pass: phase-0/unindexed
        // voxels and out-of-range ensemble indices do not participate.
        const int32 voxelPhase = phasesRef[voxelIdx];
        if(featureIdsRef[voxelIdx] == featureId && voxelPhase > 0 && static_cast<usize>(voxelPhase) < numEnsembles)
        {
          const ebsdlib::QuatD q1(quatsRef[voxelIdx * 4], quatsRef[voxelIdx * 4 + 1], quatsRef[voxelIdx * 4 + 2], quatsRef[voxelIdx * 4 + 3]);
          fzQuats.push_back(op->getFZQuat(q1)); // Fundamental Zone Reduction
        }
      }

      if(m_InputValues->useVonMisesAverage)
      {
        uint32_t seed = m_InputValues->RandomSeed;
        ebsdlib::QuatD muhat = ebsdlib::QuatD::identity();
        double kappahat = 0.0;

        if(fzQuats.size() == 1)
        {
          muhat = fzQuats[0];
        }
        else if(!fzQuats.empty())
        {
          ebsdlib::DirectionalStats directionalStats("VMF", op);
          int numEmIterations = m_InputValues->NumEMIterations;
          int numIterations = m_InputValues->NumIterations;
          directionalStats.setNumEM(numEmIterations);
          directionalStats.setNumIter(numIterations);
          directionalStats.setQuatArray(fzQuats);
          directionalStats.EMforDS(seed, muhat, kappahat, false);
          muhat.positiveOrientation();
        }

        vmfQuatPtr->setValue(featureId * 4, muhat.x());
        vmfQuatPtr->setValue(featureId * 4 + 1, muhat.y());
        vmfQuatPtr->setValue(featureId * 4 + 2, muhat.z());
        vmfQuatPtr->setValue(featureId * 4 + 3, muhat.w());

        ebsdlib::EulerDType euler = muhat.toEuler();
        vmfEulerPtr->setValue(featureId * 3, euler[0]);
        vmfEulerPtr->setValue(featureId * 3 + 1, euler[1]);
        vmfEulerPtr->setValue(featureId * 3 + 2, euler[2]);

        vmfKappaPtr->setValue(featureId, kappahat);
      }

      if(m_InputValues->useWatsonAverage)
      {
        uint32_t seed = m_InputValues->RandomSeed;
        ebsdlib::QuatD muhat = ebsdlib::QuatD::identity();
        double kappahat = 0.0;

        if(fzQuats.size() == 1)
        {
          muhat = fzQuats[0];
        }
        else if(!fzQuats.empty())
        {
          ebsdlib::DirectionalStats directionalStats("WAT", op);
          int numEmIterations = m_InputValues->NumEMIterations;
          int numIterations = m_InputValues->NumIterations;
          directionalStats.setNumEM(numEmIterations);
          directionalStats.setNumIter(numIterations);
          directionalStats.setQuatArray(fzQuats);
          directionalStats.EMforDS(seed, muhat, kappahat, false);
          muhat.positiveOrientation();
        }

        watsonQuatPtr->setValue(featureId * 4, muhat.x());
        watsonQuatPtr->setValue(featureId * 4 + 1, muhat.y());
        watsonQuatPtr->setValue(featureId * 4 + 2, muhat.z());
        watsonQuatPtr->setValue(featureId * 4 + 3, muhat.w());

        ebsdlib::EulerDType euler = muhat.toEuler();
        watsonEulerPtr->setValue(featureId * 3, euler[0]);
        watsonEulerPtr->setValue(featureId * 3 + 1, euler[1]);
        watsonEulerPtr->setValue(featureId * 3 + 2, euler[2]);

        watsonKappaPtr->setValue(featureId, kappahat);
      }

      m_Filter->sendThreadSafeProgressMessage(1);
    }
  }

private:
  ComputeAvgOrientations* m_Filter = nullptr;
  const ComputeAvgOrientationsInputValues* m_InputValues = nullptr;
  DataStructure& m_DataStructure;
  const std::vector<usize>& m_FeatureNumVoxels;
  const std::map<int32, int32>& m_FeatureIdToPhaseMap;
};

constexpr usize k_ChunkTuples = 65536;

constexpr uint64 k_OrientationRecordSize = sizeof(int32) + sizeof(uint64) + (sizeof(float32) * 4);

/**
 * @brief Multiplies sizes without overflow.
 * @param lhs Identifies the first factor.
 * @param rhs Identifies the second factor.
 * @param product Receives the product when multiplication succeeds.
 * @return True when multiplication succeeds.
 */
bool CheckedMultiply(usize lhs, usize rhs, usize& product)
{
  if(lhs != 0 && rhs > std::numeric_limits<usize>::max() / lhs)
  {
    return false;
  }
  product = lhs * rhs;
  return true;
}

/**
 * @brief Validates one Float32 feature output.
 * @param dataStructure Provides the output array.
 * @param path Identifies the output array.
 * @param tupleCount Specifies required feature tuples.
 * @param componentCount Specifies required components per tuple.
 * @return Success, or an output type or shape error.
 */
Result<> ValidateFloatOutput(DataStructure& dataStructure, const DataPath& path, usize tupleCount, usize componentCount)
{
  const auto* array = dataStructure.getDataAs<Float32Array>(path);
  if(array == nullptr)
  {
    return MakeErrorResult(-54674, fmt::format("ComputeAvgOrientations: required Float32 output array '{}' does not exist.", path.toString()));
  }
  if(array->getNumberOfTuples() != tupleCount || array->getNumberOfComponents() != componentCount)
  {
    return MakeErrorResult(-54675, fmt::format("ComputeAvgOrientations: output array '{}' has {} tuples and {} components; expected {} tuples and {} components.", path.toString(),
                                               array->getNumberOfTuples(), array->getNumberOfComponents(), tupleCount, componentCount));
  }
  return {};
}

/**
 * @brief Validates and bulk-writes one feature-level directional output.
 * @param dataStructure Provides the output array.
 * @param path Identifies the output array.
 * @param tupleCount Specifies feature tuples.
 * @param componentCount Specifies components per tuple.
 * @param values Provides contiguous output values.
 * @return Success, or an output validation or bulk-I/O error.
 *
 * Feature outputs are small relative to cell data, so one checked transfer avoids
 * repeated virtual store access without introducing cell-count scratch.
 */
Result<> WriteFloatOutput(DataStructure& dataStructure, const DataPath& path, usize tupleCount, usize componentCount, const std::vector<float32>& values)
{
  Result<> validation = ValidateFloatOutput(dataStructure, path, tupleCount, componentCount);
  if(validation.invalid())
  {
    return validation;
  }
  usize expectedValues = 0;
  if(!CheckedMultiply(tupleCount, componentCount, expectedValues) || values.size() != expectedValues)
  {
    return MakeErrorResult(
        -54676, fmt::format("ComputeAvgOrientations: output buffer for '{}' has {} values; expected {} tuples times {} components.", path.toString(), values.size(), tupleCount, componentCount));
  }
  return dataStructure.getDataRefAs<Float32Array>(path).getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(values.data(), values.size()));
}

/**
 * @struct DirectionalOutputBuffers
 * @brief Stores feature-scale vMF and Watson outputs.
 *
 * NaN values identify skipped or unsupported phases.
 */
struct DirectionalOutputBuffers
{
  DirectionalOutputBuffers(usize featureCount, bool useVmf, bool useWatson)
  {
    const float32 nan = std::numeric_limits<float32>::quiet_NaN();
    if(useVmf)
    {
      VmfQuats.assign(featureCount * 4, nan);
      VmfEuler.assign(featureCount * 3, nan);
      VmfKappa.assign(featureCount, nan);
    }
    if(useWatson)
    {
      WatsonQuats.assign(featureCount * 4, nan);
      WatsonEuler.assign(featureCount * 3, nan);
      WatsonKappa.assign(featureCount, nan);
    }
  }

  std::vector<float32> VmfQuats;
  std::vector<float32> VmfEuler;
  std::vector<float32> VmfKappa;
  std::vector<float32> WatsonQuats;
  std::vector<float32> WatsonEuler;
  std::vector<float32> WatsonKappa;
};

/**
 * @brief Stores one directional estimate in feature buffers.
 * @param featureId Identifies the output feature.
 * @param muhat Provides the quaternion estimate.
 * @param kappahat Provides the concentration estimate.
 * @param quaternions Receives four quaternion components per feature.
 * @param eulers Receives three Euler components per feature.
 * @param kappas Receives one concentration per feature.
 */
void StoreDirectionalEstimate(usize featureId, const ebsdlib::QuatD& muhat, double kappahat, std::vector<float32>& quaternions, std::vector<float32>& eulers, std::vector<float32>& kappas)
{
  const usize quaternionOffset = featureId * 4;
  quaternions[quaternionOffset] = static_cast<float32>(muhat.x());
  quaternions[quaternionOffset + 1] = static_cast<float32>(muhat.y());
  quaternions[quaternionOffset + 2] = static_cast<float32>(muhat.z());
  quaternions[quaternionOffset + 3] = static_cast<float32>(muhat.w());

  const ebsdlib::EulerDType euler = muhat.toEuler();
  const usize eulerOffset = featureId * 3;
  eulers[eulerOffset] = static_cast<float32>(euler[0]);
  eulers[eulerOffset + 1] = static_cast<float32>(euler[1]);
  eulers[eulerOffset + 2] = static_cast<float32>(euler[2]);
  kappas[featureId] = static_cast<float32>(kappahat);
}

/**
 * @brief Runs enabled directional estimators for one grouped feature.
 * @param featureId Identifies the feature.
 * @param orientationOp Provides the feature symmetry operator.
 * @param fzQuats Provides the feature fundamental-zone quaternions.
 * @param inputValues Selects estimators and their iteration limits.
 * @param shouldCancel Signals cancellation.
 * @param outputs Receives enabled feature outputs.
 * @return Success, cancellation, or an estimator error.
 *
 * EbsdLib requires a complete feature quaternion vector. Keeping one feature
 * at a time bounds simplnx grouping state. The feature vector remains the
 * largest cell-derived allocation.
 */
Result<> ComputeDirectionalFeature(usize featureId, const ebsdlib::LaueOps::Pointer& orientationOp, const std::vector<ebsdlib::QuatD>& fzQuats, const ComputeAvgOrientationsInputValues& inputValues,
                                   const std::atomic_bool& shouldCancel, DirectionalOutputBuffers& outputs)
{
  if(shouldCancel)
  {
    return {};
  }

  const auto computeEstimate = [&](const std::string& distribution, std::vector<float32>& quaternions, std::vector<float32>& eulers, std::vector<float32>& kappas) -> Result<> {
    ebsdlib::QuatD muhat = ebsdlib::QuatD::identity();
    double kappahat = 0.0;
    if(fzQuats.size() == 1)
    {
      muhat = fzQuats.front();
    }
    else if(!fzQuats.empty())
    {
      uint32 seed = inputValues.RandomSeed;
      ebsdlib::DirectionalStats directionalStats(distribution, orientationOp);
      directionalStats.setNumEM(inputValues.NumEMIterations);
      directionalStats.setNumIter(inputValues.NumIterations);
      directionalStats.setQuatArray(fzQuats);
      directionalStats.EMforDS(seed, muhat, kappahat, false);
      muhat.positiveOrientation();
    }
    if(shouldCancel)
    {
      return {};
    }
    StoreDirectionalEstimate(featureId, muhat, kappahat, quaternions, eulers, kappas);
    return {};
  };

  if(inputValues.useVonMisesAverage)
  {
    Result<> result = computeEstimate("VMF", outputs.VmfQuats, outputs.VmfEuler, outputs.VmfKappa);
    if(result.invalid() || shouldCancel)
    {
      return result;
    }
  }
  if(inputValues.useWatsonAverage)
  {
    Result<> result = computeEstimate("WAT", outputs.WatsonQuats, outputs.WatsonEuler, outputs.WatsonKappa);
    if(result.invalid() || shouldCancel)
    {
      return result;
    }
  }
  return {};
}

/**
 * @brief Encodes one fixed external-sort orientation record.
 * @param bytes Receives the fixed record bytes.
 * @param featureId Identifies the feature.
 * @param originalTupleIndex Identifies the source tuple order.
 * @param quaternion Provides four quaternion components.
 */
void EncodeOrientationRecord(nonstd::span<std::byte> bytes, int32 featureId, uint64 originalTupleIndex, const float32* quaternion)
{
  std::memcpy(bytes.data(), &featureId, sizeof(featureId));
  std::memcpy(bytes.data() + sizeof(featureId), &originalTupleIndex, sizeof(originalTupleIndex));
  std::memcpy(bytes.data() + sizeof(featureId) + sizeof(originalTupleIndex), quaternion, sizeof(float32) * 4);
}

/**
 * @brief Decodes one fixed external-sort orientation record.
 * @param bytes Provides the fixed record bytes.
 * @param featureId Receives the feature ID.
 * @param originalTupleIndex Receives the source tuple order.
 * @param quaternion Receives four quaternion components.
 */
void DecodeOrientationRecord(nonstd::span<const std::byte> bytes, int32& featureId, uint64& originalTupleIndex, std::array<float32, 4>& quaternion)
{
  std::memcpy(&featureId, bytes.data(), sizeof(featureId));
  std::memcpy(&originalTupleIndex, bytes.data() + sizeof(featureId), sizeof(originalTupleIndex));
  std::memcpy(quaternion.data(), bytes.data() + sizeof(featureId) + sizeof(originalTupleIndex), sizeof(float32) * quaternion.size());
}

/**
 * @brief Decodes an external-sort record key.
 * @param bytes Provides the fixed record bytes.
 * @param featureId Receives the feature ID.
 * @param originalTupleIndex Receives the source tuple order.
 */
void DecodeOrientationSortKey(nonstd::span<const std::byte> bytes, int32& featureId, uint64& originalTupleIndex)
{
  std::memcpy(&featureId, bytes.data(), sizeof(featureId));
  std::memcpy(&originalTupleIndex, bytes.data() + sizeof(featureId), sizeof(originalTupleIndex));
}

/**
 * @brief Orders records by feature and source tuple order.
 * @param left Provides the left record.
 * @param right Provides the right record.
 * @return A negative, zero, or positive comparison result.
 *
 * Stable source order keeps each EbsdLib feature input deterministic.
 */
int32 CompareOrientationRecords(nonstd::span<const std::byte> left, nonstd::span<const std::byte> right)
{
  int32 leftFeature = 0;
  int32 rightFeature = 0;
  uint64 leftIndex = 0;
  uint64 rightIndex = 0;
  DecodeOrientationSortKey(left, leftFeature, leftIndex);
  DecodeOrientationSortKey(right, rightFeature, rightIndex);
  if(leftFeature != rightFeature)
  {
    return leftFeature < rightFeature ? -1 : 1;
  }
  if(leftIndex == rightIndex)
  {
    return 0;
  }
  return leftIndex < rightIndex ? -1 : 1;
}
} // namespace

/**
 * @class ComputeAvgOrientations::DirectAlgorithm
 * @brief Dispatches to resident averaging.
 */
class ComputeAvgOrientations::DirectAlgorithm
{
public:
  explicit DirectAlgorithm(ComputeAvgOrientations& algorithm)
  : m_Algorithm(algorithm)
  {
  }

  Result<> operator()()
  {
    return m_Algorithm.executeDirect();
  }

private:
  ComputeAvgOrientations& m_Algorithm;
};

/**
 * @class ComputeAvgOrientations::ScanlineAlgorithm
 * @brief Dispatches to bulk-I/O and external grouping.
 */
class ComputeAvgOrientations::ScanlineAlgorithm
{
public:
  explicit ScanlineAlgorithm(ComputeAvgOrientations& algorithm)
  : m_Algorithm(algorithm)
  {
  }

  Result<> operator()()
  {
    return m_Algorithm.executeScanline();
  }

private:
  ComputeAvgOrientations& m_Algorithm;
};

ComputeAvgOrientations::ComputeAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeAvgOrientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

ComputeAvgOrientations::~ComputeAvgOrientations() noexcept = default;

void ComputeAvgOrientations::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;
  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialPoint).count() < 1000)
  {
    return;
  }

  auto progressInt = static_cast<usize>((static_cast<float32>(m_ProgressCounter) / static_cast<float32>(m_NumberOfFeatures)) * 100.0f);
  std::string ss = fmt::format("{}% Complete", progressInt);
  m_MessageHandler(IFilter::Message::Type::Info, ss);

  m_InitialPoint = std::chrono::steady_clock::now();
}

Result<> ComputeAvgOrientations::operator()()
{
  std::vector<const IArray*> targets;
  targets.reserve(13);
  const auto appendTarget = [&](const DataPath& path) {
    if(const auto* array = m_DataStructure.getDataAs<IArray>(path); array != nullptr)
    {
      targets.push_back(array);
    }
  };

  appendTarget(m_InputValues->cellFeatureIdsArrayPath);
  appendTarget(m_InputValues->cellPhasesArrayPath);
  appendTarget(m_InputValues->cellQuatsArrayPath);
  appendTarget(m_InputValues->crystalStructuresArrayPath);
  if(m_InputValues->useRodriguesAverage)
  {
    appendTarget(m_InputValues->avgQuatsArrayPath);
    appendTarget(m_InputValues->avgEulerAnglesArrayPath);
  }
  if(m_InputValues->useVonMisesAverage)
  {
    appendTarget(m_InputValues->VMFQuatsArrayPath);
    appendTarget(m_InputValues->VMFEulerAnglesArrayPath);
    appendTarget(m_InputValues->VMFKappaArrayPath);
  }
  if(m_InputValues->useWatsonAverage)
  {
    appendTarget(m_InputValues->WatsonQuatsArrayPath);
    appendTarget(m_InputValues->WatsonEulerAnglesArrayPath);
    appendTarget(m_InputValues->WatsonKappaArrayPath);
  }

  return DispatchAlgorithm<DirectAlgorithm, ScanlineAlgorithm>(AlgorithmArrayTargets(std::move(targets)), *this);
}

Result<> ComputeAvgOrientations::executeDirect()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  if(m_DataStructure.containsData(m_InputValues->avgEulerAnglesArrayPath))
  {
    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->avgEulerAnglesArrayPath, featureIds, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }
    m_NumberOfFeatures = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgEulerAnglesArrayPath).getNumberOfTuples();
  }
  else if(m_DataStructure.containsData(m_InputValues->VMFEulerAnglesArrayPath))
  {
    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->VMFEulerAnglesArrayPath, featureIds, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }
    m_NumberOfFeatures = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFEulerAnglesArrayPath).getNumberOfTuples();
  }
  else if(m_DataStructure.containsData(m_InputValues->WatsonEulerAnglesArrayPath))
  {
    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->WatsonEulerAnglesArrayPath, featureIds, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }
    m_NumberOfFeatures = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonEulerAnglesArrayPath).getNumberOfTuples();
  }
  else
  {
    return MakeErrorResult(-54670, "A valid Feature level array that stores results was not found.");
  }

  MessageHelper messageHelper(m_MessageHandler);

  Result<> finalResult;
  if(m_InputValues->useRodriguesAverage)
  {
    messageHelper.sendMessage("Computing Rodrigues Average Orientations");

    Result<> result = computeRodriguesAverage();
    if(result.invalid() || m_ShouldCancel)
    {
      return result;
    }
    finalResult = MergeResults(std::move(finalResult), std::move(result));
  }
  if(m_InputValues->useVonMisesAverage || m_InputValues->useWatsonAverage)
  {
    if(m_InputValues->useVonMisesAverage && !m_InputValues->useWatsonAverage)
    {
      messageHelper.sendMessage("Computing von-Mises Fisher Average Orientations");
    }
    if(!m_InputValues->useVonMisesAverage && m_InputValues->useWatsonAverage)
    {
      messageHelper.sendMessage("Computing Watson Average Orientations");
    }
    if(m_InputValues->useVonMisesAverage && m_InputValues->useWatsonAverage)
    {
      messageHelper.sendMessage("Computing von-Mises Fisher and Watson Average Orientations");
    }

    Result<> result = computeVmfWatsonAverage();
    if(result.invalid() || m_ShouldCancel)
    {
      return result;
    }
    finalResult = MergeResults(std::move(finalResult), std::move(result));
  }

  return finalResult;
}

Result<> ComputeAvgOrientations::executeScanline()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  if(!m_InputValues->useRodriguesAverage && !m_InputValues->useVonMisesAverage && !m_InputValues->useWatsonAverage)
  {
    return MakeErrorResult(-54670, "A valid Feature level array that stores results was not found.");
  }

  const auto* featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  const auto* phases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  const auto* quaternions = m_DataStructure.getDataAs<Float32Array>(m_InputValues->cellQuatsArrayPath);
  const auto* crystalStructures = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
  if(featureIds == nullptr || phases == nullptr || quaternions == nullptr || crystalStructures == nullptr)
  {
    return MakeErrorResult(-54674, "ComputeAvgOrientations: one or more required input arrays are missing or have the wrong scalar type.");
  }
  const usize tupleCount = featureIds->getNumberOfTuples();
  if(featureIds->getNumberOfComponents() != 1 || phases->getNumberOfComponents() != 1 || quaternions->getNumberOfComponents() != 4 || crystalStructures->getNumberOfComponents() != 1 ||
     phases->getNumberOfTuples() != tupleCount || quaternions->getNumberOfTuples() != tupleCount)
  {
    return MakeErrorResult(
        -54675,
        fmt::format("ComputeAvgOrientations: FeatureIds, Phases, and Quaternions must have identical tuple counts and component counts 1, 1, and 4; observed tuples {}/{}/{} and components {}/{}/{}.",
                    tupleCount, phases->getNumberOfTuples(), quaternions->getNumberOfTuples(), featureIds->getNumberOfComponents(), phases->getNumberOfComponents(),
                    quaternions->getNumberOfComponents()));
  }
  usize quaternionValueCount = 0;
  if(!CheckedMultiply(tupleCount, usize{4}, quaternionValueCount))
  {
    return MakeErrorResult(-54676, fmt::format("ComputeAvgOrientations: {} quaternion tuples cannot be addressed as four-component values on this platform.", tupleCount));
  }
  if(quaternions->getSize() != quaternionValueCount)
  {
    return MakeErrorResult(
        -54675, fmt::format("ComputeAvgOrientations: quaternion array '{}' has {} values; expected {} tuples times four components.", quaternions->getName(), quaternions->getSize(), tupleCount));
  }

  const DataPath* featureCountPath = nullptr;
  if(m_InputValues->useRodriguesAverage)
  {
    featureCountPath = &m_InputValues->avgEulerAnglesArrayPath;
  }
  else if(m_InputValues->useVonMisesAverage)
  {
    featureCountPath = &m_InputValues->VMFEulerAnglesArrayPath;
  }
  else
  {
    featureCountPath = &m_InputValues->WatsonEulerAnglesArrayPath;
  }
  const auto* featureCountArray = m_DataStructure.getDataAs<Float32Array>(*featureCountPath);
  if(featureCountArray == nullptr)
  {
    return MakeErrorResult(-54670, "A valid Feature level array that stores results was not found.");
  }
  m_NumberOfFeatures = featureCountArray->getNumberOfTuples();

  const auto validateOutputs = [&](bool enabled, const DataPath& quaternionPath, const DataPath& eulerPath, const DataPath* kappaPath) -> Result<> {
    if(!enabled)
    {
      return {};
    }
    Result<> result = ValidateFloatOutput(m_DataStructure, quaternionPath, m_NumberOfFeatures, 4);
    if(result.invalid())
    {
      return result;
    }
    result = ValidateFloatOutput(m_DataStructure, eulerPath, m_NumberOfFeatures, 3);
    if(result.invalid() || kappaPath == nullptr)
    {
      return result;
    }
    return ValidateFloatOutput(m_DataStructure, *kappaPath, m_NumberOfFeatures, 1);
  };
  Result<> outputResult = validateOutputs(m_InputValues->useRodriguesAverage, m_InputValues->avgQuatsArrayPath, m_InputValues->avgEulerAnglesArrayPath, nullptr);
  if(outputResult.invalid())
  {
    return outputResult;
  }
  outputResult = validateOutputs(m_InputValues->useVonMisesAverage, m_InputValues->VMFQuatsArrayPath, m_InputValues->VMFEulerAnglesArrayPath, &m_InputValues->VMFKappaArrayPath);
  if(outputResult.invalid())
  {
    return outputResult;
  }
  outputResult = validateOutputs(m_InputValues->useWatsonAverage, m_InputValues->WatsonQuatsArrayPath, m_InputValues->WatsonEulerAnglesArrayPath, &m_InputValues->WatsonKappaArrayPath);
  if(outputResult.invalid())
  {
    return outputResult;
  }

  const auto& featureIdsStore = featureIds->getDataStoreRef();
  auto featureBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  int32 minimumFeatureId = std::numeric_limits<int32>::max();
  int32 maximumFeatureId = std::numeric_limits<int32>::lowest();
  for(usize offset = 0; offset < tupleCount;)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, tupleCount - offset);
    Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto [minimum, maximum] = std::minmax_element(featureBuffer.get(), featureBuffer.get() + count);
    minimumFeatureId = std::min(minimumFeatureId, *minimum);
    maximumFeatureId = std::max(maximumFeatureId, *maximum);
    offset += count;
  }
  if(tupleCount > 0 && minimumFeatureId < 0)
  {
    return MakeErrorResult(
        -5355, fmt::format("Feature Ids array with name '{}' has negative values within the array. The most negative value encountered was '{}'. All values must be positive within the array",
                           featureIds->getName(), minimumFeatureId));
  }
  if(tupleCount > 0 && static_cast<uint64>(maximumFeatureId) >= static_cast<uint64>(m_NumberOfFeatures))
  {
    return MakeErrorResult(-5351, fmt::format("Feature Ids array with name '{}' has a value '{}' that would exceed the number of tuples {} in the selected Data Path: '{}'", featureIds->getName(),
                                              maximumFeatureId, m_NumberOfFeatures, featureCountPath->toString()));
  }

  MessageHelper messageHelper(m_MessageHandler);
  Result<> finalResult;
  if(m_InputValues->useRodriguesAverage)
  {
    messageHelper.sendMessage("Computing Rodrigues Average Orientations");
    Result<> result = computeRodriguesAverageScanline();
    if(result.invalid() || m_ShouldCancel)
    {
      return result;
    }
    finalResult = MergeResults(std::move(finalResult), std::move(result));
  }
  if(m_InputValues->useVonMisesAverage || m_InputValues->useWatsonAverage)
  {
    if(m_InputValues->useVonMisesAverage && !m_InputValues->useWatsonAverage)
    {
      messageHelper.sendMessage("Computing von-Mises Fisher Average Orientations");
    }
    else if(!m_InputValues->useVonMisesAverage && m_InputValues->useWatsonAverage)
    {
      messageHelper.sendMessage("Computing Watson Average Orientations");
    }
    else
    {
      messageHelper.sendMessage("Computing von-Mises Fisher and Watson Average Orientations");
    }
    Result<> result = computeVmfWatsonAverageScanline();
    if(result.invalid() || m_ShouldCancel)
    {
      return result;
    }
    finalResult = MergeResults(std::move(finalResult), std::move(result));
  }
  return finalResult;
}

Result<> ComputeAvgOrientations::computeVmfWatsonAverage()
{
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  const size_t totalVoxels = featureIds.getNumberOfTuples();
  const usize numEnsembles = crystalStructures.getNumberOfTuples();

  // The last valid cell phase becomes each feature's phase.
  std::vector<usize> featureNumVoxels(m_NumberOfFeatures, 0);
  std::map<int32, int32> featureIdToPhaseMap;
  usize outOfRangePhaseCount = 0;
  for(size_t i = 0; i < totalVoxels; i++)
  {
    const int32_t currentFeatureId = featureIds[i];
    const int32_t currentPhase = phases[i];
    if(currentPhase > 0)
    {
      if(static_cast<usize>(currentPhase) >= numEnsembles)
      {
        outOfRangePhaseCount++;
        continue;
      }
      featureNumVoxels[currentFeatureId]++;
      featureIdToPhaseMap[currentFeatureId] = currentPhase;
    }
  }

  usize unknownXtalFeatureCount = 0;
  const usize numValidXtal = ebsdlib::LaueOps::GetAllOrientationOps().size();
  for(const auto& [featureId, phaseValue] : featureIdToPhaseMap)
  {
    if(crystalStructures[phaseValue] >= numValidXtal)
    {
      unknownXtalFeatureCount++;
    }
  }

  Float32AbstractDataStore* vmfQuatPtr = nullptr;
  Float32AbstractDataStore* vmfEulerPtr = nullptr;
  Float32AbstractDataStore* vmfKappaPtr = nullptr;
  if(m_InputValues->useVonMisesAverage)
  {
    vmfQuatPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFQuatsArrayPath).getDataStorePtr().lock().get();
    vmfEulerPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFEulerAnglesArrayPath).getDataStorePtr().lock().get();
    vmfKappaPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFKappaArrayPath).getDataStorePtr().lock().get();
    vmfQuatPtr->fill(std::numeric_limits<float>::quiet_NaN());
    vmfEulerPtr->fill(std::numeric_limits<float>::quiet_NaN());
    vmfKappaPtr->fill(std::numeric_limits<float>::quiet_NaN());
  }

  Float32AbstractDataStore* watsonQuatPtr = nullptr;
  Float32AbstractDataStore* watsonEulerPtr = nullptr;
  Float32AbstractDataStore* watsonKappaPtr = nullptr;
  if(m_InputValues->useWatsonAverage)
  {
    watsonQuatPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonQuatsArrayPath).getDataStorePtr().lock().get();
    watsonEulerPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonEulerAnglesArrayPath).getDataStorePtr().lock().get();
    watsonKappaPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonKappaArrayPath).getDataStorePtr().lock().get();
    watsonQuatPtr->fill(std::numeric_limits<float>::quiet_NaN());
    watsonEulerPtr->fill(std::numeric_limits<float>::quiet_NaN());
    watsonKappaPtr->fill(std::numeric_limits<float>::quiet_NaN());
  }

  // Serial execution prevents concurrent access to shared DataArray and
  // DataStore objects.
  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(false);
  dataAlg.setRange(0, m_NumberOfFeatures);
  dataAlg.execute(VmfWatsonSamplingImpl(this, m_InputValues, m_DataStructure, featureNumVoxels, featureIdToPhaseMap));

  Result<> result;
  if(unknownXtalFeatureCount > 0)
  {
    result.warnings().push_back(
        {-54671, fmt::format("vMF/Watson average: {} feature(s) have an unknown/unsupported crystal structure value and were skipped; their output tuples remain NaN.", unknownXtalFeatureCount)});
  }
  if(outOfRangePhaseCount > 0)
  {
    result.warnings().push_back(
        {-54672, fmt::format("vMF/Watson average: {} cell(s) have a Phases value outside the range of the Crystal Structures array ({} tuples) and were excluded from the average.",
                             outOfRangePhaseCount, numEnsembles)});
  }
  return result;
}

Result<> ComputeAvgOrientations::computeVmfWatsonAverageScanline()
{
  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath).getDataStoreRef();
  const auto& phasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath).getDataStoreRef();
  const auto& quaternionsStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath).getDataStoreRef();
  const auto& crystalStructuresStore = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath).getDataStoreRef();
  const usize tupleCount = featureIdsStore.getNumberOfTuples();
  const usize ensembleCount = crystalStructuresStore.getNumberOfTuples();
  if(m_NumberOfFeatures > std::numeric_limits<usize>::max() / 4 || m_NumberOfFeatures > std::numeric_limits<usize>::max() / 3)
  {
    return MakeErrorResult(-54676, fmt::format("ComputeAvgOrientations: {} feature tuples cannot be addressed by the directional-statistics output component shapes.", m_NumberOfFeatures));
  }

  std::vector<uint32> crystalStructures(ensembleCount);
  if(ensembleCount > 0)
  {
    Result<> readResult = crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()));
    if(readResult.invalid())
    {
      return readResult;
    }
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  std::vector<usize> featureCounts(m_NumberOfFeatures, 0);
  std::vector<int32> winningPhases(m_NumberOfFeatures, 0);
  usize outOfRangePhaseCount = 0;
  auto featureBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  auto phaseBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < tupleCount;)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, tupleCount - offset);
    Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phaseBuffer.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    for(usize index = 0; index < count; ++index)
    {
      const int32 phase = phaseBuffer[index];
      if(phase <= 0)
      {
        continue;
      }
      if(static_cast<usize>(phase) >= ensembleCount)
      {
        ++outOfRangePhaseCount;
        continue;
      }
      const usize feature = static_cast<usize>(featureBuffer[index]);
      if(featureCounts[feature] == std::numeric_limits<usize>::max())
      {
        return MakeErrorResult(-54676, fmt::format("ComputeAvgOrientations: valid cell count for feature {} exceeds the addressable size on this platform.", feature));
      }
      ++featureCounts[feature];
      winningPhases[feature] = phase;
    }
    offset += count;
  }

  const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  std::vector<ebsdlib::LaueOps::Pointer> featureOps(m_NumberOfFeatures);
  usize unknownXtalFeatureCount = 0;
  for(usize feature = 0; feature < m_NumberOfFeatures; ++feature)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(featureCounts[feature] == 0)
    {
      continue;
    }
    const int32 phase = winningPhases[feature];
    const uint32 laueClass = crystalStructures[static_cast<usize>(phase)];
    if(laueClass >= orientationOps.size())
    {
      ++unknownXtalFeatureCount;
      continue;
    }
    featureOps[feature] = orientationOps[laueClass];
  }

  DirectionalOutputBuffers outputs(m_NumberOfFeatures, m_InputValues->useVonMisesAverage, m_InputValues->useWatsonAverage);
  const auto processFeature = [&](usize feature, const std::vector<ebsdlib::QuatD>& fzQuaternions) -> Result<> {
    if(feature >= featureOps.size() || featureOps[feature] == nullptr)
    {
      return MakeErrorResult(-54677, fmt::format("ComputeAvgOrientations: grouped quaternion data referenced feature {} without a valid selected Laue operation.", feature));
    }
    if(fzQuaternions.size() != featureCounts[feature])
    {
      return MakeErrorResult(-54678, fmt::format("ComputeAvgOrientations: grouped quaternion count for feature {} is {}; expected {} from the phase-valid counting pass.", feature,
                                                 fzQuaternions.size(), featureCounts[feature]));
    }
    Result<> result = ComputeDirectionalFeature(feature, featureOps[feature], fzQuaternions, *m_InputValues, m_ShouldCancel, outputs);
    if(result.valid() && !m_ShouldCancel)
    {
      sendThreadSafeProgressMessage(1);
    }
    return result;
  };

  if(DataStoreUtilities::GetIOCollection().hasExternalSortCapability())
  {
    ExternalSortConfig config;
    config.recordSize = k_OrientationRecordSize;
    config.maxRecordsPerBatch = k_ChunkTuples;
    config.compare = CompareOrientationRecords;
    Result<std::unique_ptr<IExternalSort>> createResult = DataStoreUtilities::GetIOCollection().createExternalSort(config);
    if(createResult.invalid())
    {
      return ConvertResult(std::move(createResult));
    }
    std::unique_ptr<IExternalSort> externalSort = std::move(createResult.value());
    if(externalSort == nullptr)
    {
      return MakeErrorResult(-54679, "ComputeAvgOrientations: the registered external-sort provider returned a null sorter.");
    }

    auto quaternionBuffer = std::make_unique<float32[]>(k_ChunkTuples * 4);
    std::vector<std::byte> recordBytes(k_ChunkTuples * static_cast<usize>(k_OrientationRecordSize));
    uint64 appendedRecordCount = 0;
    for(usize offset = 0; offset < tupleCount;)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkTuples, tupleCount - offset);
      Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.get(), count));
      if(readResult.invalid())
      {
        return readResult;
      }
      readResult = phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phaseBuffer.get(), count));
      if(readResult.invalid())
      {
        return readResult;
      }
      readResult = quaternionsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(quaternionBuffer.get(), count * 4));
      if(readResult.invalid())
      {
        return readResult;
      }
      if(m_ShouldCancel)
      {
        return {};
      }

      uint64 recordCount = 0;
      for(usize index = 0; index < count; ++index)
      {
        const int32 phase = phaseBuffer[index];
        const usize feature = static_cast<usize>(featureBuffer[index]);
        if(phase <= 0 || static_cast<usize>(phase) >= ensembleCount || featureOps[feature] == nullptr)
        {
          continue;
        }
        const usize byteOffset = static_cast<usize>(recordCount * k_OrientationRecordSize);
        EncodeOrientationRecord(nonstd::span<std::byte>(recordBytes.data() + byteOffset, static_cast<usize>(k_OrientationRecordSize)), static_cast<int32>(feature), static_cast<uint64>(offset + index),
                                quaternionBuffer.get() + (index * 4));
        ++recordCount;
      }
      if(recordCount > 0)
      {
        if(recordCount > std::numeric_limits<uint64>::max() - appendedRecordCount)
        {
          return MakeErrorResult(-54676, "ComputeAvgOrientations: external-sort record count exceeds uint64.");
        }
        Result<> appendResult = externalSort->append(recordCount, nonstd::span<const std::byte>(recordBytes.data(), static_cast<usize>(recordCount * k_OrientationRecordSize)), m_ShouldCancel, {});
        if(appendResult.invalid() || m_ShouldCancel)
        {
          return appendResult;
        }
        appendedRecordCount += recordCount;
      }
      offset += count;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    Result<> finishResult = externalSort->finish(m_ShouldCancel, {});
    if(finishResult.invalid() || m_ShouldCancel)
    {
      return finishResult;
    }

    std::optional<usize> currentFeature;
    std::optional<uint64> previousTupleIndex;
    std::vector<ebsdlib::QuatD> fzQuaternions;
    const auto flushFeature = [&]() -> Result<> {
      if(!currentFeature.has_value())
      {
        return {};
      }
      return processFeature(*currentFeature, fzQuaternions);
    };

    const uint64 totalRecords = externalSort->recordCount();
    if(totalRecords != appendedRecordCount)
    {
      return MakeErrorResult(-54680, fmt::format("ComputeAvgOrientations: external sort reports {} records after {} records were appended.", totalRecords, appendedRecordCount));
    }
    for(uint64 recordOffset = 0; recordOffset < totalRecords;)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const uint64 count = std::min<uint64>(k_ChunkTuples, totalRecords - recordOffset);
      Result<uint64> readResult = externalSort->read(recordOffset, count, nonstd::span<std::byte>(recordBytes.data(), static_cast<usize>(count * k_OrientationRecordSize)), m_ShouldCancel);
      if(readResult.invalid())
      {
        return ConvertResult(std::move(readResult));
      }
      if(readResult.value() != count)
      {
        return MakeErrorResult(-54680, fmt::format("ComputeAvgOrientations: external sort short read at record {}: requested {} records but received {}.", recordOffset, count, readResult.value()));
      }
      if(m_ShouldCancel)
      {
        return {};
      }
      for(uint64 index = 0; index < count; ++index)
      {
        int32 featureId = 0;
        uint64 originalTupleIndex = 0;
        std::array<float32, 4> quaternion{};
        const usize byteOffset = static_cast<usize>(index * k_OrientationRecordSize);
        DecodeOrientationRecord(nonstd::span<const std::byte>(recordBytes.data() + byteOffset, static_cast<usize>(k_OrientationRecordSize)), featureId, originalTupleIndex, quaternion);
        if(featureId < 0 || static_cast<usize>(featureId) >= m_NumberOfFeatures || originalTupleIndex >= tupleCount)
        {
          return MakeErrorResult(-54681, fmt::format("ComputeAvgOrientations: external sort returned invalid feature {} or original tuple index {} for {} features and {} input tuples.", featureId,
                                                     originalTupleIndex, m_NumberOfFeatures, tupleCount));
        }
        const usize feature = static_cast<usize>(featureId);
        if(!currentFeature.has_value() || *currentFeature != feature)
        {
          if(currentFeature.has_value() && feature < *currentFeature)
          {
            return MakeErrorResult(-54682, "ComputeAvgOrientations: external sort returned feature groups out of ascending order.");
          }
          Result<> flushResult = flushFeature();
          if(flushResult.invalid() || m_ShouldCancel)
          {
            return flushResult;
          }
          currentFeature = feature;
          previousTupleIndex.reset();
          fzQuaternions.clear();
          fzQuaternions.reserve(featureCounts[feature]);
        }
        if(previousTupleIndex.has_value() && originalTupleIndex <= *previousTupleIndex)
        {
          return MakeErrorResult(
              -54683, fmt::format("ComputeAvgOrientations: external sort returned non-increasing original tuple indices {} then {} for feature {}.", *previousTupleIndex, originalTupleIndex, feature));
        }
        previousTupleIndex = originalTupleIndex;
        const ebsdlib::QuatD inputQuaternion(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
        fzQuaternions.push_back(featureOps[feature]->getFZQuat(inputQuaternion));
      }
      recordOffset += count;
    }
    Result<> flushResult = flushFeature();
    if(flushResult.invalid() || m_ShouldCancel)
    {
      return flushResult;
    }
  }
  else
  {
    auto quaternionBuffer = std::make_unique<float32[]>(k_ChunkTuples * 4);
    std::vector<ebsdlib::QuatD> fzQuaternions;
    for(usize feature = 0; feature < m_NumberOfFeatures; ++feature)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      if(featureCounts[feature] == 0 || featureOps[feature] == nullptr)
      {
        continue;
      }
      fzQuaternions.clear();
      fzQuaternions.reserve(featureCounts[feature]);
      for(usize offset = 0; offset < tupleCount;)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize count = std::min(k_ChunkTuples, tupleCount - offset);
        Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.get(), count));
        if(readResult.invalid())
        {
          return readResult;
        }
        readResult = phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phaseBuffer.get(), count));
        if(readResult.invalid())
        {
          return readResult;
        }
        readResult = quaternionsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(quaternionBuffer.get(), count * 4));
        if(readResult.invalid())
        {
          return readResult;
        }
        if(m_ShouldCancel)
        {
          return {};
        }
        for(usize index = 0; index < count; ++index)
        {
          const int32 phase = phaseBuffer[index];
          if(static_cast<usize>(featureBuffer[index]) != feature || phase <= 0 || static_cast<usize>(phase) >= ensembleCount)
          {
            continue;
          }
          const usize quaternionOffset = index * 4;
          const ebsdlib::QuatD inputQuaternion(quaternionBuffer[quaternionOffset], quaternionBuffer[quaternionOffset + 1], quaternionBuffer[quaternionOffset + 2],
                                               quaternionBuffer[quaternionOffset + 3]);
          fzQuaternions.push_back(featureOps[feature]->getFZQuat(inputQuaternion));
        }
        offset += count;
      }
      Result<> featureResult = processFeature(feature, fzQuaternions);
      if(featureResult.invalid() || m_ShouldCancel)
      {
        return featureResult;
      }
    }
  }

  const auto writeOutput = [&](bool enabled, const DataPath& quaternionPath, const DataPath& eulerPath, const DataPath& kappaPath, const std::vector<float32>& quaternions,
                               const std::vector<float32>& eulers, const std::vector<float32>& kappas) -> Result<> {
    if(!enabled)
    {
      return {};
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    Result<> result = WriteFloatOutput(m_DataStructure, quaternionPath, m_NumberOfFeatures, 4, quaternions);
    if(result.invalid() || m_ShouldCancel)
    {
      return result;
    }
    result = WriteFloatOutput(m_DataStructure, eulerPath, m_NumberOfFeatures, 3, eulers);
    if(result.invalid() || m_ShouldCancel)
    {
      return result;
    }
    return WriteFloatOutput(m_DataStructure, kappaPath, m_NumberOfFeatures, 1, kappas);
  };

  Result<> writeResult = writeOutput(m_InputValues->useVonMisesAverage, m_InputValues->VMFQuatsArrayPath, m_InputValues->VMFEulerAnglesArrayPath, m_InputValues->VMFKappaArrayPath, outputs.VmfQuats,
                                     outputs.VmfEuler, outputs.VmfKappa);
  if(writeResult.invalid() || m_ShouldCancel)
  {
    return writeResult;
  }
  writeResult = writeOutput(m_InputValues->useWatsonAverage, m_InputValues->WatsonQuatsArrayPath, m_InputValues->WatsonEulerAnglesArrayPath, m_InputValues->WatsonKappaArrayPath, outputs.WatsonQuats,
                            outputs.WatsonEuler, outputs.WatsonKappa);
  if(writeResult.invalid() || m_ShouldCancel)
  {
    return writeResult;
  }

  Result<> result;
  if(unknownXtalFeatureCount > 0)
  {
    result.warnings().push_back(
        {-54671, fmt::format("vMF/Watson average: {} feature(s) have an unknown/unsupported crystal structure value and were skipped; their output tuples remain NaN.", unknownXtalFeatureCount)});
  }
  if(outOfRangePhaseCount > 0)
  {
    result.warnings().push_back(
        {-54672, fmt::format("vMF/Watson average: {} cell(s) have a Phases value outside the range of the Crystal Structures array ({} tuples) and were excluded from the average.",
                             outOfRangePhaseCount, ensembleCount)});
  }
  return result;
}

Result<> ComputeAvgOrientations::computeRodriguesAverage()
{
  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath);

  auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  auto& avgQuatsStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgQuatsArrayPath).getDataStoreRef();
  auto& avgEulerStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgEulerAnglesArrayPath).getDataStoreRef();

  const usize totalPoints = featureIds.getNumberOfTuples();
  if(totalPoints > std::numeric_limits<usize>::max() / 4)
  {
    return MakeErrorResult(-54676, fmt::format("ComputeAvgOrientations: {} quaternion tuples cannot be addressed as four-component values on this platform.", totalPoints));
  }

  const usize totalFeatures = avgQuatsStore.getNumberOfTuples();
  if(totalFeatures > std::numeric_limits<usize>::max() / 4 || totalFeatures > std::numeric_limits<usize>::max() / 3)
  {
    return MakeErrorResult(-54676, fmt::format("ComputeAvgOrientations: {} feature tuples cannot be addressed by the Rodrigues output component shapes.", totalFeatures));
  }
  std::vector<float32> counts(totalFeatures, 0.0f);
  usize outOfRangePhaseCount = 0;
  usize unknownXtalCount = 0;

  // The local ensemble cache avoids cell-loop store access.
  const usize numPhases = crystalStructuresArray.getNumberOfTuples();
  std::vector<uint32> crystalStructures(numPhases);
  if(numPhases > 0)
  {
    Result<> crystalReadResult = crystalStructuresArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), numPhases));
    if(crystalReadResult.invalid())
    {
      return crystalReadResult;
    }
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  // Feature IDs select accumulators in random order, so accumulators stay local.
  std::vector<float32> localAvgQuats(totalFeatures * 4, 0.0f);

  static const ebsdlib::QuatF identityQuat(0.0f, 0.0f, 0.0f, 1.0f);

  const auto& featureIdsStore = featureIds.getDataStoreRef();
  const auto& phasesStore = phases.getDataStoreRef();
  const auto& quatsStore = quats.getDataStoreRef();

  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto phasesBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto quatsBuf = std::make_unique<float32[]>(k_ChunkTuples * 4);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger messenger = messageHelper.createThrottledMessenger();

  for(usize offset = 0; offset < totalPoints;)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    messenger.sendThrottledMessage([offset, totalPoints]() { return fmt::format("Computing Rodrigues Average: Cell {}/{}", offset, totalPoints); });

    const usize count = std::min(k_ChunkTuples, totalPoints - offset);
    Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phasesBuf.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = quatsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(quatsBuf.get(), count * 4));
    if(readResult.invalid())
    {
      return readResult;
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    for(usize i = 0; i < count; i++)
    {
      const int32 currentFeatureId = featureIdBuf[i];
      const int32 currentPhase = phasesBuf[i];
      if(currentPhase > 0)
      {
        if(static_cast<usize>(currentPhase) >= numPhases)
        {
          outOfRangePhaseCount++;
          continue;
        }
        const uint32 xtal = crystalStructures[currentPhase];
        if(xtal >= orientationOps.size())
        {
          unknownXtalCount++;
          continue;
        }
        counts[currentFeatureId] += 1.0f;

        const usize qi = i * 4;
        ebsdlib::QuatF voxQuat(quatsBuf[qi], quatsBuf[qi + 1], quatsBuf[qi + 2], quatsBuf[qi + 3]);

        const usize fi = static_cast<usize>(currentFeatureId) * 4;
        ebsdlib::QuatF curAvgQuat(localAvgQuats[fi], localAvgQuats[fi + 1], localAvgQuats[fi + 2], localAvgQuats[fi + 3]);
        ebsdlib::QuatF finalAvgQuat = curAvgQuat;

        curAvgQuat = curAvgQuat.scalarDivide(counts[currentFeatureId]);

        if(counts[currentFeatureId] == 1.0f)
        {
          curAvgQuat = ebsdlib::QuatF::identity();
        }
        // Symmetry reduction selects the nearest running-average representation.
        voxQuat = orientationOps[xtal]->getNearestQuat(curAvgQuat, voxQuat);
        curAvgQuat = finalAvgQuat + voxQuat;

        localAvgQuats[fi] = curAvgQuat.x();
        localAvgQuats[fi + 1] = curAvgQuat.y();
        localAvgQuats[fi + 2] = curAvgQuat.z();
        localAvgQuats[fi + 3] = curAvgQuat.w();
      }
    }
    offset += count;
  }

  std::vector<float32> localAvgEuler(totalFeatures * 3, 0.0f);

  for(usize featureId = 0; featureId < totalFeatures; featureId++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize fi = featureId * 4;
    if(counts[featureId] == 0.0f)
    {
      localAvgQuats[fi] = identityQuat.x();
      localAvgQuats[fi + 1] = identityQuat.y();
      localAvgQuats[fi + 2] = identityQuat.z();
      localAvgQuats[fi + 3] = identityQuat.w();
      continue;
    }

    ebsdlib::QuatF curAvgQuat(localAvgQuats[fi], localAvgQuats[fi + 1], localAvgQuats[fi + 2], localAvgQuats[fi + 3]);
    curAvgQuat = curAvgQuat.scalarDivide(counts[featureId]);
    curAvgQuat = curAvgQuat.normalize().getPositiveOrientation();
    localAvgQuats[fi] = curAvgQuat.x();
    localAvgQuats[fi + 1] = curAvgQuat.y();
    localAvgQuats[fi + 2] = curAvgQuat.z();
    localAvgQuats[fi + 3] = curAvgQuat.w();

    ebsdlib::EulerFType eu = ebsdlib::QuaternionFType(curAvgQuat).toEuler();
    const usize ei = featureId * 3;
    localAvgEuler[ei] = eu[0];
    localAvgEuler[ei + 1] = eu[1];
    localAvgEuler[ei + 2] = eu[2];
  }

  Result<> writeResult = avgQuatsStore.copyFromBuffer(0, nonstd::span<const float32>(localAvgQuats.data(), localAvgQuats.size()));
  if(writeResult.invalid() || m_ShouldCancel)
  {
    return writeResult;
  }
  writeResult = avgEulerStore.copyFromBuffer(0, nonstd::span<const float32>(localAvgEuler.data(), localAvgEuler.size()));
  if(writeResult.invalid() || m_ShouldCancel)
  {
    return writeResult;
  }

  Result<> result;
  if(unknownXtalCount > 0)
  {
    result.warnings().push_back(
        {-54671, fmt::format("Rodrigues average: {} cell(s) belong to a phase whose crystal structure value is unknown/unsupported and were excluded from the average.", unknownXtalCount)});
  }
  if(outOfRangePhaseCount > 0)
  {
    result.warnings().push_back(
        {-54672, fmt::format("Rodrigues average: {} cell(s) have a Phases value outside the range of the Crystal Structures array ({} tuples) and were excluded from the average.",
                             outOfRangePhaseCount, numPhases)});
  }
  return result;
}

Result<> ComputeAvgOrientations::computeRodriguesAverageScanline()
{
  return computeRodriguesAverage();
}
