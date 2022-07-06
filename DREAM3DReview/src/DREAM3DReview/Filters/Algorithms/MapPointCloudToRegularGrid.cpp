#include "MapPointCloudToRegularGrid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
MapPointCloudToRegularGrid::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        MapPointCloudToRegularGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MapPointCloudToRegularGrid::~MapPointCloudToRegularGrid() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& MapPointCloudToRegularGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> MapPointCloudToRegularGrid::operator()()
{

  return {};
}
