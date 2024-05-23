#include "ComputeSlipTransmissionMetrics.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"

#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeSlipTransmissionMetrics::ComputeSlipTransmissionMetrics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               ComputeSlipTransmissionMetricsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSlipTransmissionMetrics::~ComputeSlipTransmissionMetrics() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeSlipTransmissionMetrics::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeSlipTransmissionMetrics::operator()()
{
  auto orientationOps = LaueOps::GetAllOrientationOps();

  auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  usize totalFeatures = featurePhases.getNumberOfTuples();

  auto& neighborList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListArrayPath);

  std::vector<std::vector<float32>> F1Lists(totalFeatures);
  std::vector<std::vector<float32>> F1sPtLists(totalFeatures);
  std::vector<std::vector<float32>> F7Lists(totalFeatures);
  std::vector<std::vector<float32>> mPrimeLists(totalFeatures);

  float64 LD[3] = {0.0, 0.0, 1.0};

  int32 nName;
  float32 mPrime, F1, F1sPt, F7;

  bool emitLaueClassWarning = false;

  for(usize i = 1; i < totalFeatures; i++)
  {
    usize listLength = neighborList[i].size();
    F1Lists[i].assign(listLength, 0.0f);
    F1sPtLists[i].assign(listLength, 0.0f);
    F7Lists[i].assign(listLength, 0.0f);
    mPrimeLists[i].assign(listLength, 0.0f);
    for(usize j = 0; j < listLength; j++)
    {
      nName = neighborList[i][j];
      QuatD q1(avgQuats[i * 4], avgQuats[i * 4 + 1], avgQuats[i * 4 + 2], avgQuats[i * 4 + 3]);
      QuatD q2(avgQuats[nName * 4], avgQuats[nName * 4 + 1], avgQuats[nName * 4 + 2], avgQuats[nName * 4 + 3]);

      uint32 laueClassI = static_cast<uint32>(featurePhases[i]);
      uint32 laueClassN = static_cast<uint32>(featurePhases[nName]);

      if(laueClassI == laueClassN && laueClassN != 1)
      {
        emitLaueClassWarning = true;
      }
      // Make sure we only run the algorithm on CubicOps: orientationOps[1];
      if(crystalStructures[laueClassI] == crystalStructures[laueClassN] && featurePhases[i] > 0 && laueClassN == 1)
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

  auto& F1L = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F1ListArrayName);
  auto& F1sptL = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F1sptListArrayName);
  auto& F7L = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F7ListArrayName);
  auto& mPrimeL = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->mPrimeListArrayName);

  for(usize i = 1; i < totalFeatures; i++)
  {
    Float32NeighborList::SharedVectorType f1L(new std::vector<float32>(F1Lists[i]));
    F1L.setList(static_cast<int32>(i), f1L);

    Float32NeighborList::SharedVectorType f1sptL(new std::vector<float32>(F1sPtLists[i]));
    F1sptL.setList(static_cast<int32>(i), f1sptL);

    Float32NeighborList::SharedVectorType f7L(new std::vector<float32>(F7Lists[i]));
    F7L.setList(static_cast<int32>(i), f7L);

    Float32NeighborList::SharedVectorType primeL(new std::vector<float32>(mPrimeLists[i]));
    mPrimeL.setList(static_cast<int32>(i), primeL);
  }

  if(emitLaueClassWarning)
  {
    return MakeWarningVoidResult(-94739, fmt::format("A phase other then Cubic m-3m is being analyzed. This filter only works on Cubic m-3m Laue classes. Those phases have a result of 0.0."));
  }

  return {};
}
