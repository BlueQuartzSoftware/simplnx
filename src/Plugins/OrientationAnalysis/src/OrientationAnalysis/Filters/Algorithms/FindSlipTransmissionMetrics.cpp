#include "FindSlipTransmissionMetrics.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/NeighborList.hpp"

#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace complex;

// -----------------------------------------------------------------------------
FindSlipTransmissionMetrics::FindSlipTransmissionMetrics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         FindSlipTransmissionMetricsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindSlipTransmissionMetrics::~FindSlipTransmissionMetrics() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindSlipTransmissionMetrics::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindSlipTransmissionMetrics::operator()()
{
  auto orientationOps = LaueOps::GetAllOrientationOps();

  auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

  usize totalFeatures = featurePhases.getNumberOfTuples();

  // But since a pointer is difficult to use operators with we will now create a
  // reference variable to the pointer with the correct variable name that allows
  // us to use the same syntax as the "vector of vectors"
  auto& neighborList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListArrayPath);

  auto& F1Lists = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F1ListArrayName);
  auto& F1sPtLists = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F1sptListArrayName);
  auto& F7Lists = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F7ListArrayName);
  auto& mPrimeLists = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->mPrimeListArrayName);

  float32 mPrime, F1, F1sPt, F7;
  int32 nName;

  float64 LD[3] = {0.0, 0.0, 1.0};
  for(usize i = 1; i < totalFeatures; i++)
  {
    F1Lists[i].assign(neighborList[i].size(), 0.0f);
    F1sPtLists[i].assign(neighborList[i].size(), 0.0f);
    F7Lists[i].assign(neighborList[i].size(), 0.0f);
    mPrimeLists[i].assign(neighborList[i].size(), 0.0f);
    for(usize j = 0; j < neighborList[i].size(); j++)
    {
      nName = neighborList[i][j];
      QuatD q1(avgQuats[i * 4], avgQuats[i * 4 + 1], avgQuats[i * 4 + 2], avgQuats[i * 4 + 3]);
      QuatD q2(avgQuats[nName * 4], avgQuats[nName * 4 + 1], avgQuats[nName * 4 + 2], avgQuats[nName * 4 + 3]);

      if(crystalStructures[featurePhases[i]] == crystalStructures[featurePhases[nName]] && featurePhases[i] > 0)
      {
        mPrime = static_cast<float32>(orientationOps[crystalStructures[featurePhases[i]]]->getmPrime(q1, q2, LD));
        F1 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[i]]]->getF1(q1, q2, LD, true));
        F1sPt = static_cast<float32>(orientationOps[crystalStructures[featurePhases[i]]]->getF1spt(q1, q2, LD, true));
        F7 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[i]]]->getF7(q1, q2, LD, true));
      }
      else
      {
        mPrime = 0.0f;
        F1 = 0.0f;
        F1sPt = 0.0f;
        F7 = 0.0f;
      }
      mPrimeLists[i][j] = mPrime;
      F1Lists[i][j] = F1;
      F1sPtLists[i][j] = F1sPt;
      F7Lists[i][j] = F7;
    }
  }

  return {};
}
