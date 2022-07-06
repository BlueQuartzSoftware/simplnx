#include "GenerateFaceMisorientationColoring.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GenerateFaceMisorientationColoring::GenerateFaceMisorientationColoring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       GenerateFaceMisorientationColoringInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateFaceMisorientationColoring::~GenerateFaceMisorientationColoring() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateFaceMisorientationColoring::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateFaceMisorientationColoring::operator()()
{

  return {};
}
