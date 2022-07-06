#include "EMsoftSO3Sampler.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
EMsoftSO3Sampler::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EMsoftSO3SamplerInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
EMsoftSO3Sampler::~EMsoftSO3Sampler() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& EMsoftSO3Sampler::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> EMsoftSO3Sampler::operator()()
{

  return {};
}
