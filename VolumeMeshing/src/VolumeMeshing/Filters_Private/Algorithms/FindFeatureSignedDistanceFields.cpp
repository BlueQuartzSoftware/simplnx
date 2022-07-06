#include "FindFeatureSignedDistanceFields.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindFeatureSignedDistanceFields::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             FindFeatureSignedDistanceFieldsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindFeatureSignedDistanceFields::~FindFeatureSignedDistanceFields() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindFeatureSignedDistanceFields::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindFeatureSignedDistanceFields::operator()()
{

  return {};
}
