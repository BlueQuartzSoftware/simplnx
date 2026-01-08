#include "ComputeBoundaryElementFractions.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBoundaryElementFractions::ComputeBoundaryElementFractions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 ComputeBoundaryElementFractionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundaryElementFractions::~ComputeBoundaryElementFractions() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeBoundaryElementFractions::operator()()
{

  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& boundaryCells = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->BoundaryCellsArrayPath);
  auto& boundaryCellFractions = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureDataAttributeMatrixPath.createChildPath(m_InputValues->BoundaryCellFractionsArrayName));

  usize totalPoints = featureIds.getNumberOfTuples();
  usize numFeatures = boundaryCellFractions.getNumberOfTuples();

  std::vector<float32> surfVoxCounts(numFeatures, 0);
  std::vector<float32> voxCounts(numFeatures, 0);

  for(usize j = 0; j < totalPoints; j++)
  {
    int32 gnum = featureIds[j];
    voxCounts[gnum]++;
    if(boundaryCells[j] > 0)
    {
      surfVoxCounts[gnum]++;
    }
  }
  for(usize i = 1; i < numFeatures; i++)
  {
    boundaryCellFractions[i] = surfVoxCounts[i] / voxCounts[i];
  }
  return {};
}
