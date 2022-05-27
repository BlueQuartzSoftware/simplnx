#include "FindAvgOrientations.hpp"

#include "complex/DataStructure/DataStore.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindAvgOrientations::FindAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindAvgOrientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindAvgOrientations::~FindAvgOrientations() noexcept = default;

// -----------------------------------------------------------------------------
Result<> FindAvgOrientations::operator()()
{

  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  complex::Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  complex::Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  complex::Float32Array& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath);

  complex::UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  complex::Float32Array& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgQuatsArrayPath);
  complex::Float32Array& avgEuler = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgEulerAnglesArrayPath);

  size_t totalPoints = featureIds.getNumberOfTuples();

  // Since we don't have the notion of an AttributeMatrix (Yet) we are going to run through the featureIds
  // and find the unique set of values, this will tell us the how to size the output arrays.
  // This also ASSUMES that the feature Ids are continuous from ZERO to some MAX value. That MAX value
  // should also be the size. These are the prerequisite for the algorithm to work.
  std::set<int32> uniqueFeatureIds;
  for(const auto& featureId : featureIds)
  {
    uniqueFeatureIds.insert(featureId);
  }
  int32 maxFeatureId = 1 + *std::max_element(std::begin(uniqueFeatureIds), std::end(uniqueFeatureIds));
  size_t totalFeatures = uniqueFeatureIds.size();

  if(totalFeatures != maxFeatureId)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-87000, fmt::format("The total number of unique features is {} but the max featureID is {}.", totalFeatures, maxFeatureId)}})};
  }

  std::vector<float> counts(totalFeatures, 0.0f);

  // Resize the output arrays and initialize
  avgQuats.getDataStore()->reshapeTuples({totalFeatures});
  avgQuats.fill(0.0F);
  // Initialize all Euler Angles to Zero
  avgEuler.getDataStore()->reshapeTuples({totalFeatures});
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

      // Get the pointer to the current voxel's Quaternion
      QuatF voxQuat(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]); // Makes a copy into voxQuat!!!!
      QuatF nearestQuat = orientationOps[crystalStructures[phase]]->getNearestQuat(curAvgQuat, voxQuat);

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
