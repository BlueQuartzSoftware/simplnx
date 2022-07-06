#include "ApproximatePointCloudHull.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ApproximatePointCloudHull::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                       ApproximatePointCloudHullInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApproximatePointCloudHull::~ApproximatePointCloudHull() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ApproximatePointCloudHull::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ApproximatePointCloudHull::operator()()
{

  return {};
}
