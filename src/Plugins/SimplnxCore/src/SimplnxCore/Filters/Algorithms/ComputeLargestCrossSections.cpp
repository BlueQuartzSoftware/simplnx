#include "ComputeLargestCrossSections.hpp"

#include "ComputeLargestCrossSectionsDirect.hpp"
#include "ComputeLargestCrossSectionsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeLargestCrossSections::ComputeLargestCrossSections(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ComputeLargestCrossSectionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeLargestCrossSections::~ComputeLargestCrossSections() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeLargestCrossSections::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeLargestCrossSections::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& largestCrossSections = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->LargestCrossSectionsArrayPath);
  return DispatchAlgorithm<ComputeLargestCrossSectionsDirect, ComputeLargestCrossSectionsScanline>({&featureIds, &largestCrossSections}, m_DataStructure, m_MessageHandler, m_ShouldCancel,
                                                                                                   m_InputValues);
}
