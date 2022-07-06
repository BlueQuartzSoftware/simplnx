#include "FindVertexToTriangleDistances.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindVertexToTriangleDistances::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                           FindVertexToTriangleDistancesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindVertexToTriangleDistances::~FindVertexToTriangleDistances() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindVertexToTriangleDistances::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindVertexToTriangleDistances::operator()()
{

  return {};
}
