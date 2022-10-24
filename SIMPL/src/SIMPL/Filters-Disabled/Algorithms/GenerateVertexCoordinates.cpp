#include "GenerateVertexCoordinates.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GenerateVertexCoordinates::GenerateVertexCoordinates(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     GenerateVertexCoordinatesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateVertexCoordinates::~GenerateVertexCoordinates() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateVertexCoordinates::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateVertexCoordinates::operator()()
{

  return {};
}
