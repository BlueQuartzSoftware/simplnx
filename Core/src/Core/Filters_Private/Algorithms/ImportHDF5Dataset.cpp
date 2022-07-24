#include "ImportHDF5Dataset.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportHDF5Dataset::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportHDF5DatasetInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportHDF5Dataset::~ImportHDF5Dataset() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportHDF5Dataset::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportHDF5Dataset::operator()()
{

  return {};
}
