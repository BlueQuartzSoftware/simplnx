#include "ItkBinaryWatershedLabeled.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ItkBinaryWatershedLabeled::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                       ItkBinaryWatershedLabeledInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ItkBinaryWatershedLabeled::~ItkBinaryWatershedLabeled() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ItkBinaryWatershedLabeled::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ItkBinaryWatershedLabeled::operator()()
{

  return {};
}
