#include "FindFeaturePhasesBinary.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindFeaturePhasesBinary::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeaturePhasesBinaryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindFeaturePhasesBinary::~FindFeaturePhasesBinary() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindFeaturePhasesBinary::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindFeaturePhasesBinary::operator()()
{

  return {};
}
