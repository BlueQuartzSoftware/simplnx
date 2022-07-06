#include "InitializeSyntheticVolume.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
InitializeSyntheticVolume::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                       InitializeSyntheticVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InitializeSyntheticVolume::~InitializeSyntheticVolume() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& InitializeSyntheticVolume::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InitializeSyntheticVolume::operator()()
{

  return {};
}
