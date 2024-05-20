#include "ComputeAvgOrientations.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeAvgOrientations::ComputeAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeAvgOrientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeAvgOrientations::~ComputeAvgOrientations() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeAvgOrientations::operator()()
{

  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  nx::core::Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  nx::core::Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  nx::core::Float32Array& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath);

  nx::core::UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  nx::core::Float32Array& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgQuatsArrayPath);
  nx::core::Float32Array& avgEuler = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgEulerAnglesArrayPath);

  size_t totalPoints = featureIds.getNumberOfTuples();

  auto numFeatResults = ValidateNumFeaturesInArray(m_DataStructure, m_InputValues->avgQuatsArrayPath, featureIds);
  if(numFeatResults.invalid())
  {
    return numFeatResults;
  }
  size_t totalFeatures = avgQuats.getNumberOfTuples();
  std::vector<float> counts(totalFeatures, 0.0f);

  // initialize the output arrays
  avgQuats.fill(0.0F);
  // Initialize all Euler Angles to Zero
  avgEuler.fill(0.0F);

  // Get the DataStore for each output array
  auto* avgQuatDataStore = avgQuats.getDataStore();

  // Get the Identity Quaternion
  std::array<float, 4> identityQuat = {0.0F, 0.0F, 0.0F, 1.0F};

  // Initialize all Average Quats to Identity
  for(size_t tupleIndex = 1; tupleIndex < totalFeatures; tupleIndex++)
  {
    avgQuatDataStore->setTuple(tupleIndex, identityQuat);
  }

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(featureIds[i] > 0 && phases[i] > 0)
    {
      int32 phase = phases[i];
      size_t featureId = static_cast<size_t>(featureIds[i]);

      size_t featureIdOffset = featureId * 4;
      counts[featureId] += 1.0f;

      float32 count = counts[featureId];
      QuatF curAvgQuat(avgQuats[featureIdOffset] / count, avgQuats[featureIdOffset + 1] / count, avgQuats[featureIdOffset + 2] / count, avgQuats[featureIdOffset + 3] / count);

      // Make a copy of the current quaternion from the DataArray into a QuatF object
      QuatF voxQuat(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
      QuatF nearestQuat = orientationOps[crystalStructures[phase]]->getNearestQuat(curAvgQuat, voxQuat);

      // Add the running average quat with the current quat
      curAvgQuat = curAvgQuat + nearestQuat;
      // Copy the new curAvgQuat back into the output array
      avgQuats[featureIdOffset] = curAvgQuat.x();
      avgQuats[featureIdOffset + 1] = curAvgQuat.y();
      avgQuats[featureIdOffset + 2] = curAvgQuat.z();
      avgQuats[featureIdOffset + 3] = curAvgQuat.w();
    }
  }

  for(size_t featureId = 1; featureId < totalFeatures; featureId++)
  {
    size_t featureIdOffset = featureId * 4;
    float32 count = counts[featureId];

    // Create a copy of the quaternion
    QuatF curAvgQuat(avgQuats[featureIdOffset] / count, avgQuats[featureIdOffset + 1] / count, avgQuats[featureIdOffset + 2] / count, avgQuats[featureIdOffset + 3] / count);
    curAvgQuat = curAvgQuat.unitQuaternion();

    avgQuats[featureIdOffset] = curAvgQuat.x();
    avgQuats[featureIdOffset + 1] = curAvgQuat.y();
    avgQuats[featureIdOffset + 2] = curAvgQuat.z();
    avgQuats[featureIdOffset + 3] = curAvgQuat.w();

    OrientationF eu = OrientationTransformation::qu2eu<Quaternion<float>, Orientation<float>>(curAvgQuat);
    featureIdOffset = featureId * 3;
    avgEuler[featureIdOffset] = eu[0];
    avgEuler[featureIdOffset + 1] = eu[1];
    avgEuler[featureIdOffset + 2] = eu[2];
  }

  return {};
}
