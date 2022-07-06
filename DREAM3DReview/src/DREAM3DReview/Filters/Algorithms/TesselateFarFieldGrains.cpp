#include "TesselateFarFieldGrains.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
TesselateFarFieldGrains::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TesselateFarFieldGrainsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TesselateFarFieldGrains::~TesselateFarFieldGrains() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& TesselateFarFieldGrains::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> TesselateFarFieldGrains::operator()()
{

  return {};
}
