#include "ComputeUmeyamaTransform.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ComputeUmeyamaTransform::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeUmeyamaTransformInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeUmeyamaTransform::~ComputeUmeyamaTransform() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeUmeyamaTransform::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeUmeyamaTransform::operator()()
{

  return {};
}
