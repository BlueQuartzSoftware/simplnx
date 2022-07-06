#include "InterpolatePointCloudToRegularGrid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

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
const std::atomic_bool& InterpolatePointCloudToRegularGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGrid::operator()()
{

  return {};
}
