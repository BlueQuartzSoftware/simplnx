#include "ImportQMMeltpoolH5File.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportQMMeltpoolH5File::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportQMMeltpoolH5FileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportQMMeltpoolH5File::~ImportQMMeltpoolH5File() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportQMMeltpoolH5File::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportQMMeltpoolH5File::operator()()
{

  return {};
}
