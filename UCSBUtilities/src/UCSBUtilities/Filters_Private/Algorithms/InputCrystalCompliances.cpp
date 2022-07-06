#include "InputCrystalCompliances.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
InputCrystalCompliances::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InputCrystalCompliancesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InputCrystalCompliances::~InputCrystalCompliances() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& InputCrystalCompliances::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InputCrystalCompliances::operator()()
{

  return {};
}
