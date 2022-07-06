#include "ITKImportRoboMetMontage.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ITKImportRoboMetMontage::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImportRoboMetMontageInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ITKImportRoboMetMontage::~ITKImportRoboMetMontage() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ITKImportRoboMetMontage::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ITKImportRoboMetMontage::operator()()
{

  return {};
}
