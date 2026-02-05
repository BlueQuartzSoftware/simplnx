#include "InterpolatePointCloudToRegularGrid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
InterpolatePointCloudToRegularGrid::InterpolatePointCloudToRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       InterpolatePointCloudToRegularGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InterpolatePointCloudToRegularGrid::~InterpolatePointCloudToRegularGrid() noexcept = default;

// -----------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGrid::operator()()
{

  return {};
}
