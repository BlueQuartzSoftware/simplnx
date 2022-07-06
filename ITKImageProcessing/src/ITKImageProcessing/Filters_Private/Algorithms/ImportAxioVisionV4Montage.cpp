#include "ImportAxioVisionV4Montage.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportAxioVisionV4Montage::ImportAxioVisionV4Montage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ImportAxioVisionV4MontageInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportAxioVisionV4Montage::~ImportAxioVisionV4Montage() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportAxioVisionV4Montage::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportAxioVisionV4Montage::operator()()
{

  return {};
}
