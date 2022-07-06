#include "AverageEdgeFaceCellArrayToVertexArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AverageEdgeFaceCellArrayToVertexArray::AverageEdgeFaceCellArrayToVertexArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                             AverageEdgeFaceCellArrayToVertexArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AverageEdgeFaceCellArrayToVertexArray::~AverageEdgeFaceCellArrayToVertexArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AverageEdgeFaceCellArrayToVertexArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AverageEdgeFaceCellArrayToVertexArray::operator()()
{

  return {};
}
