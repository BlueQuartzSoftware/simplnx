#include "AverageVertexArrayToEdgeFaceCellArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AverageVertexArrayToEdgeFaceCellArray::AverageVertexArrayToEdgeFaceCellArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                             AverageVertexArrayToEdgeFaceCellArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AverageVertexArrayToEdgeFaceCellArray::~AverageVertexArrayToEdgeFaceCellArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AverageVertexArrayToEdgeFaceCellArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AverageVertexArrayToEdgeFaceCellArray::operator()()
{

  return {};
}
