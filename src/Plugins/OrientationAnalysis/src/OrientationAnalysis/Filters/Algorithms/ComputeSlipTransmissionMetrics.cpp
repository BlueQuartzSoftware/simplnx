#include "ComputeSlipTransmissionMetrics.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"

#include <EbsdLib/EbsdLibVersion.h>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/Quaternion.hpp>

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
  const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const auto& avgQuatsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto& featurePhasesArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& crystalStructuresArrayRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  const usize totalFeatures = featurePhasesArrayRef.getNumberOfTuples();
  const usize numCrystalStructures = crystalStructuresArrayRef.getNumberOfTuples();

  auto& neighborList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListArrayPath);

  std::vector<std::vector<float32>> F1Lists(totalFeatures);
  std::vector<std::vector<float32>> F1sPtLists(totalFeatures);
  std::vector<std::vector<float32>> F7Lists(totalFeatures);
  std::vector<std::vector<float32>> mPrimeLists(totalFeatures);

  float64 LD[3] = {0.0, 0.0, 1.0};

  bool emitLaueClassWarning = false;

  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize listLength = neighborList[featureIdx].size();
    F1Lists[featureIdx].assign(listLength, 0.0F);
    F1sPtLists[featureIdx].assign(listLength, 0.0F);
    F7Lists[featureIdx].assign(listLength, 0.0F);
    mPrimeLists[featureIdx].assign(listLength, 0.0F);
    for(usize neighborIdx = 0; neighborIdx < listLength; neighborIdx++)
    {
      const int32 neighborFeatureIdx = neighborList[featureIdx][neighborIdx];
      const ebsdlib::QuatD q1(avgQuatsArrayRef[featureIdx * 4], avgQuatsArrayRef[featureIdx * 4 + 1], avgQuatsArrayRef[featureIdx * 4 + 2], avgQuatsArrayRef[featureIdx * 4 + 3]);
      const ebsdlib::QuatD q2(avgQuatsArrayRef[neighborFeatureIdx * 4], avgQuatsArrayRef[neighborFeatureIdx * 4 + 1], avgQuatsArrayRef[neighborFeatureIdx * 4 + 2],
                              avgQuatsArrayRef[neighborFeatureIdx * 4 + 3]);

      const int32 currentPhaseIdx = featurePhasesArrayRef[featureIdx];
      const int32 neighborFeaturePhaseIdx = featurePhasesArrayRef[neighborFeatureIdx];

      if(currentPhaseIdx == neighborFeaturePhaseIdx && neighborFeaturePhaseIdx != 1)
      {
        emitLaueClassWarning = true;
      }

      float32 mPrime = 0.0F;
      float32 F1 = 0.0F;
      float32 F1sPt = 0.0F;
      float32 F7 = 0.0F;

      // The algorithm calculates metrics only when the current Phase is positive and the neighbor belongs to Phase 1.
      if(currentPhaseIdx > 0 && neighborFeaturePhaseIdx == 1)
      {
        if(static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
        {
          return MakeErrorResult(
              -94742, fmt::format("Feature Phases array '{}' has value {} at Feature index {}, but Crystal Structures array '{}' contains {} tuples. Valid positive Phase indices are in [1, {}).",
                                  m_InputValues->FeaturePhasesArrayPath.toString(), currentPhaseIdx, featureIdx, m_InputValues->CrystalStructuresArrayPath.toString(), numCrystalStructures,
                                  numCrystalStructures));
        }
        if(static_cast<usize>(neighborFeaturePhaseIdx) >= numCrystalStructures)
        {
          return MakeErrorResult(
              -94742,
              fmt::format("Feature Phases array '{}' has value {} at neighbor Feature index {}, but Crystal Structures array '{}' contains {} tuples. Valid positive Phase indices are in [1, {}).",
                          m_InputValues->FeaturePhasesArrayPath.toString(), neighborFeaturePhaseIdx, neighborFeatureIdx, m_InputValues->CrystalStructuresArrayPath.toString(), numCrystalStructures,
                          numCrystalStructures));
        }

        const uint32 currentLaueIndex = crystalStructuresArrayRef[currentPhaseIdx];
        const uint32 neighborLaueIndex = crystalStructuresArrayRef[neighborFeaturePhaseIdx];
        if(currentLaueIndex == neighborLaueIndex)
        {
          if(currentLaueIndex >= orientationOps.size())
          {
            return MakeErrorResult(-94743, fmt::format("Crystal Structures array '{}' has value {} at Phase index {}, but only {} Laue operations are available. Valid Laue indices are in [0, {}).",
                                                       m_InputValues->CrystalStructuresArrayPath.toString(), currentLaueIndex, currentPhaseIdx, orientationOps.size(), orientationOps.size()));
          }
          mPrime = static_cast<float32>(orientationOps[currentLaueIndex]->getmPrime(q1, q2, LD));
          F1 = static_cast<float32>(orientationOps[currentLaueIndex]->getF1(q1, q2, LD, true));
          F1sPt = static_cast<float32>(orientationOps[currentLaueIndex]->getF1spt(q1, q2, LD, true));
          F7 = static_cast<float32>(orientationOps[currentLaueIndex]->getF7(q1, q2, LD, true));
        }
      }
      mPrimeLists[featureIdx][neighborIdx] = mPrime;
      F1Lists[featureIdx][neighborIdx] = F1;
      F1sPtLists[featureIdx][neighborIdx] = F1sPt;
      F7Lists[featureIdx][neighborIdx] = F7;
    }
  }

  auto& F1L = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F1ListArrayName);
  auto& F1sptL = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F1sptListArrayName);
  auto& F7L = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->F7ListArrayName);
  auto& mPrimeL = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->mPrimeListArrayName);

  F1L.setLists(F1Lists);
  F1sptL.setLists(F1sPtLists);
  F7L.setLists(F7Lists);
  mPrimeL.setLists(mPrimeLists);

  // for(usize i = 1; i < totalFeatures; i++)
  //{
  //   Float32NeighborList::SharedVectorType f1L(new std::vector<float32>(F1Lists[i]));
  //   F1L.setList(static_cast<int32>(i), f1L);

  //  Float32NeighborList::SharedVectorType f1sptL(new std::vector<float32>(F1sPtLists[i]));
  //  F1sptL.setList(static_cast<int32>(i), f1sptL);

  //  Float32NeighborList::SharedVectorType f7L(new std::vector<float32>(F7Lists[i]));
  //  F7L.setList(static_cast<int32>(i), f7L);

  //  Float32NeighborList::SharedVectorType primeL(new std::vector<float32>(mPrimeLists[i]));
  //  mPrimeL.setList(static_cast<int32>(i), primeL);
  //}

  if(emitLaueClassWarning)
  {
    return MakeWarningVoidResult(-94739, "A Phase other than Cubic m-3m is being analyzed. This filter only works on Cubic m-3m Laue classes. Those Phases have a result of 0.0.");
  }

  return {};
}
