#include "ReadHDF5Dataset.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ReadHDF5Dataset::ReadHDF5Dataset(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadHDF5DatasetInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadHDF5Dataset::~ReadHDF5Dataset() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadHDF5Dataset::operator()()
{

  return {};
}
