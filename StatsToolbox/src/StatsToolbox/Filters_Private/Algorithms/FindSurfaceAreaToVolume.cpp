#include "FindSurfaceAreaToVolume.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindSurfaceAreaToVolume::FindSurfaceAreaToVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 FindSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindSurfaceAreaToVolume::~FindSurfaceAreaToVolume() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindSurfaceAreaToVolume::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindSurfaceAreaToVolume::operator()()
{

  return {};
}
