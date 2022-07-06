#include "FindNeighborListStatistics.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindNeighborListStatistics::FindNeighborListStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       FindNeighborListStatisticsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindNeighborListStatistics::~FindNeighborListStatistics() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindNeighborListStatistics::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindNeighborListStatistics::operator()()
{

  return {};
}
