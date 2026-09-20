#include "ComputeFeatureNeighborMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeFeatureNeighborMisorientations::ComputeFeatureNeighborMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                             ComputeFeatureNeighborMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighborMisorientations::~ComputeFeatureNeighborMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureNeighborMisorientations::operator()()
{

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Input Arrays
  const auto& featurePhasesArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& avgQuatsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto& crystalStructuresArrayRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const auto& neighborListRef = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborListArrayPath);
  const usize numCrystalStructures = crystalStructuresArrayRef.getNumberOfTuples();

  // The output misorientations is going to be used as a vector because the output array is optional and might
  // not exist in the DataStructure. We cannot get it by reference.
  auto* avgMisorientationsArrayPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgMisorientationsArrayName);

  const usize totalFeatures = featurePhasesArrayRef.getNumberOfTuples();

  std::vector<std::vector<float>> tempMisorientationLists(totalFeatures);
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    usize quatIndex = featureIdx * 4;

    ebsdlib::QuatD q1(avgQuatsArrayRef[quatIndex], avgQuatsArrayRef[quatIndex + 1], avgQuatsArrayRef[quatIndex + 2], avgQuatsArrayRef[quatIndex + 3]);
    const int32 currentPhaseIdx = featurePhasesArrayRef[featureIdx];
    if(currentPhaseIdx < 0 || static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
    {
      return MakeErrorResult(-34502, fmt::format("Feature Phases array '{}' has value {} at feature index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in [0, {}).",
                                                 m_InputValues->FeaturePhasesArrayPath.toString(), currentPhaseIdx, featureIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                                 numCrystalStructures, numCrystalStructures));
    }
    const uint32 currentLaueIndex = crystalStructuresArrayRef[currentPhaseIdx];

    const NeighborList<int32_t>::VectorType featureNeighborList = neighborListRef.at(static_cast<int32_t>(featureIdx));

    tempMisorientationLists[featureIdx].assign(featureNeighborList.size(), -1.0);
    // The divisor counts only matched neighbor pairs. Its feature-local lifetime prevents mismatch deductions from affecting other features.
    size_t tempMisoList = featureNeighborList.size();

    for(usize neighborListIdx = 0; neighborListIdx < featureNeighborList.size(); neighborListIdx++)
    {
      const int32 neighborFeatureIdx = featureNeighborList[neighborListIdx];
      quatIndex = neighborFeatureIdx * 4;
      ebsdlib::QuatD q2(avgQuatsArrayRef[quatIndex], avgQuatsArrayRef[quatIndex + 1], avgQuatsArrayRef[quatIndex + 2], avgQuatsArrayRef[quatIndex + 3]);
      const int32 neighborFeaturePhaseIdx = featurePhasesArrayRef[neighborFeatureIdx];
      if(neighborFeaturePhaseIdx < 0 || static_cast<usize>(neighborFeaturePhaseIdx) >= numCrystalStructures)
      {
        return MakeErrorResult(-34502, fmt::format("Feature Phases array '{}' has value {} at feature index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in [0, {}).",
                                                   m_InputValues->FeaturePhasesArrayPath.toString(), neighborFeaturePhaseIdx, neighborFeatureIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                                   numCrystalStructures, numCrystalStructures));
      }
      const uint32 neighborLaueIndex = crystalStructuresArrayRef[neighborFeaturePhaseIdx];
      if(currentLaueIndex == neighborLaueIndex && currentLaueIndex < orientationOps.size())
      {
        ebsdlib::AxisAngleDType axisAngle = orientationOps[currentLaueIndex]->calculateMisorientation(q1, q2);

        tempMisorientationLists[featureIdx][neighborListIdx] = static_cast<float>(axisAngle[3] * nx::core::Constants::k_180OverPiF);
        if(m_InputValues->ComputeAvgMisors)
        {
          (*avgMisorientationsArrayPtr)[featureIdx] += tempMisorientationLists[featureIdx][neighborListIdx];
        }
      }
      else
      {
        if(m_InputValues->ComputeAvgMisors)
        {
          tempMisoList > 0 ? tempMisoList-- : tempMisoList = 0;
        }
        tempMisorientationLists[featureIdx][neighborListIdx] = NAN;
      }
    }
    if(m_InputValues->ComputeAvgMisors)
    {
      if(tempMisoList != 0)
      {
        (*avgMisorientationsArrayPtr)[featureIdx] /= static_cast<float>(tempMisoList);
      }
      else
      {
        (*avgMisorientationsArrayPtr)[featureIdx] = NAN;
      }
    }
  }

  // Output Variables
  auto& outMisorientationList = m_DataStructure.getDataRefAs<NeighborList<float32>>(m_InputValues->MisorientationListArrayName);
  // Set the vector for each list into the NeighborList Object
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    // Construct a shared vector<float> through the std::vector<> copy constructor.
    NeighborList<float>::SharedVectorType sharedMisorientationList(new std::vector<float>(tempMisorientationLists[featureIdx]));
    outMisorientationList.setList(static_cast<int32_t>(featureIdx), sharedMisorientationList);
  }

  return {};
}
