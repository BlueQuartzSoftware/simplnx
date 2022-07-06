#include "ImportPrintRiteHDF5File.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportPrintRiteHDF5File::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportPrintRiteHDF5FileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportPrintRiteHDF5File::~ImportPrintRiteHDF5File() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportPrintRiteHDF5File::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportPrintRiteHDF5File::operator()()
{

  return {};
}
