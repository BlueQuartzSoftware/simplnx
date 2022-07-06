#include "FindNeighborhoods.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindNeighborhoods::FindNeighborhoods(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindNeighborhoodsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindNeighborhoods::~FindNeighborhoods() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindNeighborhoods::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindNeighborhoods::operator()()
{

  return {};
}
