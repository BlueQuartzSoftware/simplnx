#include "NearestPointFuseRegularGrids.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
NearestPointFuseRegularGrids::NearestPointFuseRegularGrids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           NearestPointFuseRegularGridsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
NearestPointFuseRegularGrids::~NearestPointFuseRegularGrids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& NearestPointFuseRegularGrids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> NearestPointFuseRegularGrids::operator()()
{

  return {};
}
