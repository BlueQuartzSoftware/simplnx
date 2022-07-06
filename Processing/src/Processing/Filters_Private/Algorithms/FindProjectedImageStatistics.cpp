#include "FindProjectedImageStatistics.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindProjectedImageStatistics::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          FindProjectedImageStatisticsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindProjectedImageStatistics::~FindProjectedImageStatistics() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindProjectedImageStatistics::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindProjectedImageStatistics::operator()()
{

  return {};
}
