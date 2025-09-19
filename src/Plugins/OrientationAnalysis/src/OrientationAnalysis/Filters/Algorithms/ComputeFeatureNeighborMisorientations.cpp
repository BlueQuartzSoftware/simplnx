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
const std::atomic_bool& ComputeFeatureNeighborMisorientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureNeighborMisorientations::operator()()
{

  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  // Input Arrays
  const auto& inFeaturePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& inAvgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto& inXtalStruct = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const auto& inNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborListArrayPath);

  // The output misorientations is going to be used as a vector because the output array is optional and might
  // not exist in the DataStructure. We cannot get it by reference.
  auto* avgMisorientations = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgMisorientationsArrayName);

  size_t tempMisoList = 0;

  size_t totalFeatures = inFeaturePhases.getNumberOfTuples();

  std::vector<std::vector<float>> tempMisorientationLists(totalFeatures);
  usize quatIndex = 0;
  for(size_t i = 1; i < totalFeatures; i++)
  {
    quatIndex = i * 4;

    QuatF q1(inAvgQuats[quatIndex], inAvgQuats[quatIndex + 1], inAvgQuats[quatIndex + 2], inAvgQuats[quatIndex + 3]);
    uint32_t laueClass1 = inXtalStruct[inFeaturePhases[i]];

    const NeighborList<int32_t>::VectorType featureNeighborList = inNeighborList.at(static_cast<int32_t>(i));

    tempMisorientationLists[i].assign(featureNeighborList.size(), -1.0);

    for(size_t j = 0; j < featureNeighborList.size(); j++)
    {
      int32_t neighborFeatureId = featureNeighborList[j];
      quatIndex = neighborFeatureId * 4;
      QuatF q2(inAvgQuats[quatIndex], inAvgQuats[quatIndex + 1], inAvgQuats[quatIndex + 2], inAvgQuats[quatIndex + 3]);
      uint32_t xtalType2 = inXtalStruct[inFeaturePhases[neighborFeatureId]];
      tempMisoList = featureNeighborList.size();
      if(laueClass1 == xtalType2 && static_cast<int64_t>(laueClass1) < static_cast<int64_t>(orientationOps.size()))
      {
        OrientationD axisAngle = orientationOps[laueClass1]->calculateMisorientation(q1, q2);

        tempMisorientationLists[i][j] = static_cast<float>(axisAngle[3] * nx::core::Constants::k_180OverPiF);
        if(m_InputValues->ComputeAvgMisors)
        {
          (*avgMisorientations)[i] += tempMisorientationLists[i][j];
        }
      }
      else
      {
        if(m_InputValues->ComputeAvgMisors)
        {
          tempMisoList > 0 ? tempMisoList-- : tempMisoList = 0;
        }
        tempMisorientationLists[i][j] = NAN;
      }
    }
    if(m_InputValues->ComputeAvgMisors)
    {
      if(tempMisoList != 0)
      {
        (*avgMisorientations)[i] /= static_cast<float>(tempMisoList);
      }
      else
      {
        (*avgMisorientations)[i] = NAN;
      }
      tempMisoList = 0;
    }
  }

  // Output Variables
  auto& outMisorientationList = m_DataStructure.getDataRefAs<NeighborList<float32>>(m_InputValues->MisorientationListArrayName);
  // Set the vector for each list into the NeighborList Object
  for(size_t i = 1; i < totalFeatures; i++)
  {
    // Construct a shared vector<float> through the std::vector<> copy constructor.
    NeighborList<float>::SharedVectorType sharedMisorientationList(new std::vector<float>(tempMisorientationLists[i]));
    outMisorientationList.setList(static_cast<int32_t>(i), sharedMisorientationList);
  }

  return {};
}
