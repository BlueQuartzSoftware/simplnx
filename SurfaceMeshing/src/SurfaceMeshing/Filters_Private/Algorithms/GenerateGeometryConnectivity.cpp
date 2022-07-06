#include "GenerateGeometryConnectivity.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GenerateGeometryConnectivity::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          GenerateGeometryConnectivityInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateGeometryConnectivity::~GenerateGeometryConnectivity() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateGeometryConnectivity::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateGeometryConnectivity::operator()()
{

  return {};
}
