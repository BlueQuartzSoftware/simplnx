#include "ImportDeformKeyFilev12Filter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportDeformKeyFilev12Filter::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          ImportDeformKeyFilev12FilterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportDeformKeyFilev12Filter::~ImportDeformKeyFilev12Filter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportDeformKeyFilev12Filter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportDeformKeyFilev12Filter::operator()()
{

  return {};
}
