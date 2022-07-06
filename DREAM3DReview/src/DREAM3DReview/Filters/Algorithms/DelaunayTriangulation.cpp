#include "DelaunayTriangulation.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
DelaunayTriangulation::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DelaunayTriangulationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DelaunayTriangulation::~DelaunayTriangulation() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& DelaunayTriangulation::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> DelaunayTriangulation::operator()()
{

  return {};
}
