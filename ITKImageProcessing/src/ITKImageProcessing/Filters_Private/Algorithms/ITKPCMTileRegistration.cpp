#include "ITKPCMTileRegistration.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ITKPCMTileRegistration::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKPCMTileRegistrationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ITKPCMTileRegistration::~ITKPCMTileRegistration() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ITKPCMTileRegistration::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ITKPCMTileRegistration::operator()()
{

  return {};
}
