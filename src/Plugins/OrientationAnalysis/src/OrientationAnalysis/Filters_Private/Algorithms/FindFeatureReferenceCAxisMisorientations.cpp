#include "FindFeatureReferenceCAxisMisorientations.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindFeatureReferenceCAxisMisorientations::FindFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                   FindFeatureReferenceCAxisMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindFeatureReferenceCAxisMisorientations::~FindFeatureReferenceCAxisMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindFeatureReferenceCAxisMisorientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindFeatureReferenceCAxisMisorientations::operator()()
{

  return {};
}
