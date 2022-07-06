#include "InsertTransformationPhases.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
InsertTransformationPhases::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        InsertTransformationPhasesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InsertTransformationPhases::~InsertTransformationPhases() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& InsertTransformationPhases::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InsertTransformationPhases::operator()()
{

  return {};
}
