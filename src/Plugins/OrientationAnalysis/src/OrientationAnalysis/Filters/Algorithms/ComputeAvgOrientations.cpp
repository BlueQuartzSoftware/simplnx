#include "ComputeAvgOrientations.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/DirectionalStats.hpp>
#include <EbsdLib/LaueOps/LaueOps.h>

#include <iostream>

using namespace nx::core;

namespace
{

class VmfWatsonSamplingImpl
{
public:
  VmfWatsonSamplingImpl(ComputeAvgOrientations* filter, const ComputeAvgOrientationsInputValues* inputPtr, DataStructure& dataStructure, const std::vector<usize>& featureNumVoxels,
                        const std::map<int32, int32>& featureIdToPhaseMap)
  : m_Filter(filter)
  , m_InputValues(inputPtr)
  , m_DataStructure(dataStructure)
  , m_FeatureNumVoxels(featureNumVoxels)
  , m_FeatureIdToPhaseMap(featureIdToPhaseMap)
  {
  }

  virtual ~VmfWatsonSamplingImpl() = default;

  void operator()(const Range& range) const
  {
    // Input FeatureIds + Input Orientations. All these should come from the same Attribute Matrix or have the same number of tuples
    Int32AbstractDataStore& featureIdsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath).getDataStoreRef();
    Int32AbstractDataStore& phasesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath).getDataStoreRef();
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
    const usize numEnsembles = xtalRef.getNumberOfTuples();

    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();

    std::vector<ebsdlib::QuatD> fzQuats;
    for(usize featureId = range.min(); featureId < range.max(); featureId++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      // If the size is 0 then skip to the next feature
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

      // Loop over every "voxel" (although the user could just be passing in an array
      // they want to find the average orientation of
      for(usize voxelIdx = 0; voxelIdx < numVoxels; voxelIdx++)
      {
        // If the feature Id of the voxel matches the current feature Id, then grab that orientation.
        // The phase gate MUST match the counting pass in computeVmfWatsonAverage() (and the
        // Rodrigues path): phase-0/unindexed voxels are excluded from the average (issue #1659),
        // and an out-of-range phase index would read past the CrystalStructures array (issue #1661).
        const int32 voxelPhase = phasesRef[voxelIdx];
        if(featureIdsRef[voxelIdx] == featureId && voxelPhase > 0 && static_cast<usize>(voxelPhase) < numEnsembles)
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
  m_MessageHandler.sendInfoMessage(ss);

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

  // Warnings (e.g. dropped voxels/features) from each path are merged and returned together.
  Result<> finalResult;
  if(m_InputValues->useRodriguesAverage)
  {
    messageHelper.sendMessage("Computing Rodrigues Average Orientations");

    Result<> result = computeRodriguesAverage();
    if(result.invalid())
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
    if(result.invalid())
    {
      return result;
    }
    finalResult = MergeResults(std::move(finalResult), std::move(result));
  }

  return finalResult;
}

// -----------------------------------------------------------------------------
Result<> ComputeAvgOrientations::computeVmfWatsonAverage()
{
  // Input Data
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  const size_t totalVoxels = featureIds.getNumberOfTuples();
  const usize numEnsembles = crystalStructures.getNumberOfTuples();

  // Run through the "voxels" and compute the number of voxels for each feature.
  // NOTE: for a feature spanning multiple phases the map is last-writer-wins — the
  // feature's crystal structure is taken from the phase of its highest-index voxel.
  // (vMF/Watson uses one phase per feature; the Rodrigues path is per-voxel.)
  std::vector<usize> featureNumVoxels(m_NumberOfFeatures, 0);
  std::map<int32, int32> featureIdToPhaseMap;
  usize outOfRangePhaseCount = 0;
  for(size_t i = 0; i < totalVoxels; i++)
  {
    const int32_t currentFeatureId = featureIds[i];
    const int32_t currentPhase = phases[i];
    if(currentPhase > 0)
    {
      // An out-of-range phase index would read past the CrystalStructures array (issue #1661).
      // This gate MUST match the gather loop in VmfWatsonSamplingImpl.
      if(static_cast<usize>(currentPhase) >= numEnsembles)
      {
        outOfRangePhaseCount++;
        continue;
      }
      featureNumVoxels[currentFeatureId]++;
      featureIdToPhaseMap[currentFeatureId] = currentPhase;
    }
  }

  // Features whose crystal structure is unknown/unsupported are skipped by the worker
  // (their outputs remain NaN); report that up front instead of dropping them silently.
  usize unknownXtalFeatureCount = 0;
  {
    const usize numValidXtal = ebsdlib::LaueOps::GetAllOrientationOps().size();
    for(const auto& [featureId, phaseValue] : featureIdToPhaseMap)
    {
      if(crystalStructures[phaseValue] >= numValidXtal)
      {
        unknownXtalFeatureCount++;
      }
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

  // NOTE: Parallelization is intentionally DISABLED. The worker reads the shared cell
  // FeatureIds/Quats/CrystalStructures DataStores and writes the per-feature output
  // DataStores; per the simplnx thread-safety policy, DataArray/DataStore access is not
  // safe for concurrent use even at distinct indices. Serial execution is the correct
  // default (see vv/ComputeAvgOrientationsFilter.md and ComputeFeatureFaceMisorientation).
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

// -----------------------------------------------------------------------------
Result<> ComputeAvgOrientations::computeRodriguesAverage()
{
  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  Float32Array& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath);

  UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgQuatsArrayPath).getDataStoreRef();
  auto& avgEuler = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgEulerAnglesArrayPath).getDataStoreRef();

  const size_t totalPoints = featureIds.getNumberOfTuples();
  const usize numEnsembles = crystalStructures.getNumberOfTuples();

  size_t totalFeatures = avgQuats.getNumberOfTuples();
  std::vector<float> counts(totalFeatures, 0.0f);
  usize outOfRangePhaseCount = 0;
  usize unknownXtalCount = 0;

  // initialize the output arrays
  avgQuats.fill(0.0F);
  // Initialize all Euler Angles to Zero
  avgEuler.fill(0.0F);

  // Get the Identity Quaternion
  static const ebsdlib::QuatF identityQuat(0.0f, 0.0f, 0.0f, 1.0f);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger messenger = messageHelper.createThrottledMessenger();

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    messenger.sendThrottledMessage([i, totalPoints]() { return fmt::format("Computing Rodrigues Average: Cell {}/{}", i + 1, totalPoints); });

    const int32_t currentFeatureId = featureIds[i];
    const int32_t currentPhase = phases[i];
    // As long as we have a valid `currentPhase` value which is used as an index
    // into the CrystalStructures array. We ALWAYS ignore the first value in the
    // CrystalStructures array. So therefore the `currentPhase` MUST be > 0.
    // We can use `currentFeatureId = 0` because if someone is just wanting to compute
    // the average of a bunch of orientations they may have labeled the "FeatureIds = 0"
    // for all the values. The most important value is the `currentPhase` which for
    // the grand majority of historical data should be > 0.
    //    Now in theory someone could absolutely manually import data into an "Ensemble"
    // Array and NOT have the zero index as `unknown` in which case this check will
    // fail them and they will not compute anything most likely. The documentation
    // for the filter should be updated to cover these use-cases.
    if(currentPhase > 0)
    {
      // An out-of-range phase index would read past the CrystalStructures array (issue #1661).
      if(static_cast<usize>(currentPhase) >= numEnsembles)
      {
        outOfRangePhaseCount++;
        continue;
      }
      const uint32 xtal = crystalStructures[currentPhase];
      // Guard against an out-of-range crystal-structure enum (e.g. 999/Unknown).
      if(xtal >= orientationOps.size())
      {
        unknownXtalCount++;
        continue;
      }
      counts[currentFeatureId] += 1.0f;
      ebsdlib::QuatF voxQuat(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
      ebsdlib::QuatF curAvgQuat(avgQuats[currentFeatureId * 4], avgQuats[currentFeatureId * 4 + 1], avgQuats[currentFeatureId * 4 + 2], avgQuats[currentFeatureId * 4 + 3]);
      ebsdlib::QuatF finalAvgQuat(avgQuats[currentFeatureId * 4], avgQuats[currentFeatureId * 4 + 1], avgQuats[currentFeatureId * 4 + 2], avgQuats[currentFeatureId * 4 + 3]);

      curAvgQuat = curAvgQuat.scalarDivide(counts[currentFeatureId]);

      if(counts[currentFeatureId] == 1.0f)
      {
        curAvgQuat = ebsdlib::QuatF::identity();
      }
      voxQuat = orientationOps[xtal]->getNearestQuat(curAvgQuat, voxQuat);
      curAvgQuat = finalAvgQuat + voxQuat;

      UpdateQuaternionArray(avgQuats, curAvgQuat, currentFeatureId);
    }
  }

  for(size_t featureId = 0; featureId < totalFeatures; featureId++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(counts[featureId] == 0.0f)
    {
      UpdateQuaternionArray(avgQuats, identityQuat, featureId);
      continue;
    }

    ebsdlib::QuatF curAvgQuat(avgQuats[featureId * 4], avgQuats[featureId * 4 + 1], avgQuats[featureId * 4 + 2], avgQuats[featureId * 4 + 3]);
    curAvgQuat = curAvgQuat.scalarDivide(counts[featureId]);
    curAvgQuat = curAvgQuat.normalize().getPositiveOrientation(); // Be sure the Quaterion is in the Northern Hemisphere
    UpdateQuaternionArray(avgQuats, curAvgQuat, featureId);

    // Update the value for the average Euler.
    ebsdlib::EulerFType eu = ebsdlib::QuaternionFType(curAvgQuat).toEuler();
    UpdateEulerArray(avgEuler, eu, featureId);
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
                             outOfRangePhaseCount, numEnsembles)});
  }
  return result;
}
