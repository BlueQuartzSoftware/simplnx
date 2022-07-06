#include "IlluminationCorrection.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
IlluminationCorrection::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IlluminationCorrectionInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IlluminationCorrection::~IlluminationCorrection() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& IlluminationCorrection::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> IlluminationCorrection::operator()()
{

  return {};
}
