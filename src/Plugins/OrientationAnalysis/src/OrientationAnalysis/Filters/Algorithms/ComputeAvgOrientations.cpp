#include "ComputeAvgOrientations.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/DirectionalStats.hpp>
#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

#include <memory>

using namespace nx::core;

namespace
{

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
    // Input FeatureIds + Input Orientations. All these should come from the same Attribute Matrix or have the same number of tuples
    Int32AbstractDataStore& featureIdsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath).getDataStoreRef();
    // Int32AbstractDataStore& phasesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath).getDataStoreRef();
    Float32AbstractDataStore& quatsRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath).getDataStoreRef();
    // Ensemble Level Data
    UInt32AbstractDataStore& xtalRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath).getDataStoreRef();

    // Output vMF Data
    Float32AbstractDataStore* vmfQuatPtr = nullptr;
    Float32AbstractDataStore* vmfEulerPtr = nullptr;
    Float32AbstractDataStore* vmfKappaPtr = nullptr;
    if(m_InputValues->useVonMisesAverage)
    {
      vmfQuatPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFQuatsArrayPath).getDataStorePtr().lock().get();
      vmfEulerPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFEulerAnglesArrayPath).getDataStorePtr().lock().get();
      vmfKappaPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VMFKappaArrayPath).getDataStorePtr().lock().get();
    }

    // Output Watson Data
    Float32AbstractDataStore* watsonQuatPtr = nullptr;
    Float32AbstractDataStore* watsonEulerPtr = nullptr;
    Float32AbstractDataStore* watsonKappaPtr = nullptr;
    if(m_InputValues->useWatsonAverage)
    {
      watsonQuatPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonQuatsArrayPath).getDataStorePtr().lock().get();
      watsonEulerPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonEulerAnglesArrayPath).getDataStorePtr().lock().get();
      watsonKappaPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->WatsonKappaArrayPath).getDataStorePtr().lock().get();
    }

    usize numVoxels = featureIdsRef.getNumberOfTuples();

    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();

    std::vector<ebsdlib::QuatD> fzQuats;
    for(usize featureId = range.min(); featureId < range.max(); featureId++)
    {
      // If the size is 0 then skip to the next feature
      if(m_FeatureNumVoxels[featureId] == 0)
      {
        continue;
      }

      const int32 phaseIdx = m_FeatureIdToPhaseMap.at(static_cast<int32>(featureId));
      const uint32 laueClass = xtalRef[phaseIdx];
      ebsdlib::LaueOps::Pointer op = ops[laueClass];

      fzQuats.clear();
      fzQuats.reserve(m_FeatureNumVoxels[featureId]);

      // Loop over every "voxel" (although the user could just be passing in an array
      // they want to find the average orientation of
      for(usize voxelIdx = 0; voxelIdx < numVoxels; voxelIdx++)
      {
        // If the feature Id of the voxel matches the current feature Id, then grab that orientation
        if(featureIdsRef[voxelIdx] == featureId)
        {
          const ebsdlib::QuatD q1(quatsRef[voxelIdx * 4], quatsRef[voxelIdx * 4 + 1], quatsRef[voxelIdx * 4 + 2], quatsRef[voxelIdx * 4 + 3]);
          fzQuats.push_back(op->getFZQuat(q1)); // Fundamental Zone Reduction
        }
      }

      // Now that we have all the orientations for the given featureId we can compute the averages
      if(m_InputValues->useVonMisesAverage)
      {
        uint32_t seed = m_InputValues->RandomSeed; // This should be a user facing options
        ebsdlib::QuatD muhat = ebsdlib::QuatD::identity();
        double kappahat = 0.0;

        if(fzQuats.size() == 1)
        {
          muhat = fzQuats[0];
        }
        else if(!fzQuats.empty())
        {
          ebsdlib::DirectionalStats directionalStats("VMF", op);
          int numEmIterations = m_InputValues->NumEMIterations; // At some point this should be a user-defined input
          int numIterations = m_InputValues->NumIterations;     // At some point this should be a user-defined input
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
        uint32_t seed = m_InputValues->RandomSeed; // This should be a user facing options
        ebsdlib::QuatD muhat = ebsdlib::QuatD::identity();
        double kappahat = 0.0;

        // Check if there is only a single orientation...
        if(fzQuats.size() == 1)
        {
          muhat = fzQuats[0];
        }
        else if(!fzQuats.empty())
        {
          ebsdlib::DirectionalStats directionalStats("WAT", op);
          int numEmIterations = m_InputValues->NumEMIterations; // At some point this should be a user-defined input
          int numIterations = m_InputValues->NumIterations;     // At some point this should be a user-defined input
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

template <typename T>
void UpdateQuaternionArray(AbstractDataStore<T>& quatArray, const ebsdlib::Quaternion<T>& quat, int32 tupleIndex)
{
  quatArray.setValue(tupleIndex * 4, quat.x());
  quatArray.setValue(tupleIndex * 4 + 1, quat.y());
  quatArray.setValue(tupleIndex * 4 + 2, quat.z());
  quatArray.setValue(tupleIndex * 4 + 3, quat.w());
}

template <typename T>
void UpdateEulerArray(AbstractDataStore<T>& eulerArray, const ebsdlib::Euler<T>& euler, int32 tupleIndex)
{
  eulerArray.setValue(tupleIndex * 3, euler[0]);
  eulerArray.setValue(tupleIndex * 3 + 1, euler[1]);
  eulerArray.setValue(tupleIndex * 3 + 2, euler[2]);
}

constexpr usize k_ChunkTuples = 65536;
} // namespace

// -----------------------------------------------------------------------------
ComputeAvgOrientations::ComputeAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeAvgOrientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
ComputeAvgOrientations::~ComputeAvgOrientations() noexcept = default;

// -----------------------------------------------------------------------------
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

  m_LastProgressInt = progressInt;
  m_InitialPoint = std::chrono::steady_clock::now();
}

// -----------------------------------------------------------------------------
Result<> ComputeAvgOrientations::operator()()
{
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

  Result<> result;
  if(m_InputValues->useRodriguesAverage)
  {
    messageHelper.sendMessage("Computing Rodrigues Average Orientations");

    result = computeRodriguesAverage();
    if(result.invalid())
    {
      return result;
    }
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

    result = computeVmfWatsonAverage();
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> ComputeAvgOrientations::computeVmfWatsonAverage()
{
  // Input Data
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);

  const size_t totalVoxels = featureIds.getNumberOfTuples();

  // Run through the "voxels" and compute the number of voxels for each feature
  std::vector<usize> featureNumVoxels(m_NumberOfFeatures, 0);
  std::map<int32, int32> featureIdToPhaseMap;
  for(size_t i = 0; i < totalVoxels; i++)
  {
    const int32_t currentFeatureId = featureIds[i];
    const int32_t currentPhase = phases[i];
    if(currentPhase > 0)
    {
      featureNumVoxels[currentFeatureId]++;
      featureIdToPhaseMap[currentFeatureId] = currentPhase;
    }
  }

  // Initialize the output arrays
  // Output vMF Data
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

  // Output Watson Data
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

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, m_NumberOfFeatures);
  dataAlg.execute(VmfWatsonSamplingImpl(this, m_InputValues, m_DataStructure, featureNumVoxels, featureIdToPhaseMap));

  return {};
}

// -----------------------------------------------------------------------------
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

  const usize totalFeatures = avgQuatsStore.getNumberOfTuples();
  std::vector<float32> counts(totalFeatures, 0.0f);

  // Cache crystal structures locally (ensemble-level, tiny)
  const usize numPhases = crystalStructuresArray.getNumberOfTuples();
  std::vector<uint32> crystalStructures(numPhases);
  crystalStructuresArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), numPhases));

  // Local cache for avgQuats (feature-level, manageable)
  std::vector<float32> localAvgQuats(totalFeatures * 4, 0.0f);

  // Get the Identity Quaternion
  static const ebsdlib::QuatF identityQuat(0.0f, 0.0f, 0.0f, 1.0f);

  // Chunked accumulation of cell-level data
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  const auto& phasesStore = phases.getDataStoreRef();
  const auto& quatsStore = quats.getDataStoreRef();

  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto phasesBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto quatsBuf = std::make_unique<float32[]>(k_ChunkTuples * 4);

  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, totalPoints - offset);
    featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phasesBuf.get(), count));
    quatsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(quatsBuf.get(), count * 4));

    for(usize i = 0; i < count; i++)
    {
      const int32 currentFeatureId = featureIdBuf[i];
      const int32 currentPhase = phasesBuf[i];
      if(currentFeatureId > 0 && currentPhase > 0)
      {
        const uint32 xtal = crystalStructures[currentPhase];
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
        voxQuat = orientationOps[xtal]->getNearestQuat(curAvgQuat, voxQuat);
        curAvgQuat = finalAvgQuat + voxQuat;

        localAvgQuats[fi] = curAvgQuat.x();
        localAvgQuats[fi + 1] = curAvgQuat.y();
        localAvgQuats[fi + 2] = curAvgQuat.z();
        localAvgQuats[fi + 3] = curAvgQuat.w();
      }
    }
  }

  // Second pass: normalize and convert to Euler angles (feature-level only)
  std::vector<float32> localAvgEuler(totalFeatures * 3, 0.0f);

  for(usize featureId = 1; featureId < totalFeatures; featureId++)
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

  // Write feature-level results back to DataStore
  avgQuatsStore.copyFromBuffer(0, nonstd::span<const float32>(localAvgQuats.data(), localAvgQuats.size()));
  avgEulerStore.copyFromBuffer(0, nonstd::span<const float32>(localAvgEuler.data(), localAvgEuler.size()));

  return {};
}
