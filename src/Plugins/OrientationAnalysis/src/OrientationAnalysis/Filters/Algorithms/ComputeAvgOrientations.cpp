#include "ComputeAvgOrientations.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

namespace
{

std::ostream& operator<<(std::ostream& os, const QuatF& q)
{
  os << ", " << q.x() << ", " << q.y() << ", " << q.z() << ", " << q.w();
  return os;
}

std::ostream& operator<<(std::ostream& os, const QuatD& q)
{
  os << ", " << q.x() << ", " << q.y() << ", " << q.z() << ", " << q.w();
  return os;
}

template <typename T>
void UpdateQuaternionArray(AbstractDataStore<T>& quatArray, const Quaternion<T>& quat, int32 tupleIndex)
{
  quatArray.setValue(tupleIndex * 4, quat.x());
  quatArray.setValue(tupleIndex * 4 + 1, quat.y());
  quatArray.setValue(tupleIndex * 4 + 2, quat.z());
  quatArray.setValue(tupleIndex * 4 + 3, quat.w());
}

template <typename T>
void UpdateEulerArray(AbstractDataStore<T>& eulerArray, const Orientation<T>& euler, int32 tupleIndex)
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
Result<> ComputeAvgOrientations::operator()()
{
  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellFeatureIdsArrayPath);
  Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  Float32Array& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellQuatsArrayPath);

  UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgQuatsArrayPath).getDataStoreRef();
  auto& avgEuler = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgEulerAnglesArrayPath).getDataStoreRef();

  const size_t totalPoints = featureIds.getNumberOfTuples();

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->avgQuatsArrayPath, featureIds, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }
  size_t totalFeatures = avgQuats.getNumberOfTuples();
  std::vector<float> counts(totalFeatures, 0.0f);

  // initialize the output arrays
  avgQuats.fill(0.0F);
  // Initialize all Euler Angles to Zero
  avgEuler.fill(0.0F);

  // Get the Identity Quaternion
  static const QuatF identityQuat(0.0f, 0.0f, 0.0f, 1.0f);

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const int32_t currentFeatureId = featureIds[i];
    const int32_t currentPhase = phases[i];
    if(currentFeatureId > 0 && currentPhase > 0)
    {
      const uint32 xtal = crystalStructures[currentPhase];
      counts[currentFeatureId] += 1.0f;
      QuatF voxQuat(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
      QuatF curAvgQuat(avgQuats[currentFeatureId * 4], avgQuats[currentFeatureId * 4 + 1], avgQuats[currentFeatureId * 4 + 2], avgQuats[currentFeatureId * 4 + 3]);
      QuatF finalAvgQuat(avgQuats[currentFeatureId * 4], avgQuats[currentFeatureId * 4 + 1], avgQuats[currentFeatureId * 4 + 2], avgQuats[currentFeatureId * 4 + 3]);

      curAvgQuat = curAvgQuat.scalarDivide(counts[currentFeatureId]);

      if(counts[currentFeatureId] == 1.0f)
      {
        curAvgQuat = QuatF::identity();
      }
      voxQuat = orientationOps[xtal]->getNearestQuat(curAvgQuat, voxQuat);
      curAvgQuat = finalAvgQuat + voxQuat;

      UpdateQuaternionArray(avgQuats, curAvgQuat, currentFeatureId);
    }
  }

  for(size_t featureId = 1; featureId < totalFeatures; featureId++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(counts[featureId] == 0.0f)
    {
      UpdateQuaternionArray(avgQuats, identityQuat, featureId);
    }

    QuatF curAvgQuat(avgQuats[featureId * 4], avgQuats[featureId * 4 + 1], avgQuats[featureId * 4 + 2], avgQuats[featureId * 4 + 3]);
    curAvgQuat = curAvgQuat.scalarDivide(counts[featureId]);
    curAvgQuat = curAvgQuat.unitQuaternion();

    UpdateQuaternionArray(avgQuats, curAvgQuat, featureId);

    // Update the value for the average Euler. Be sure to 'normalize' the Quaterion
    // before converting it to a Euler Angle
    OrientationF eu = OrientationTransformation::qu2eu<QuatF, OrientationF>(curAvgQuat.normalize());
    UpdateEulerArray(avgEuler, eu, featureId);
  }

  return {};
}
