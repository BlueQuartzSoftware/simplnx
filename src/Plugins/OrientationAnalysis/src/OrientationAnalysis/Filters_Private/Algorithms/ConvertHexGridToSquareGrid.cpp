#include "ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ConvertHexGridToSquareGrid::ConvertHexGridToSquareGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ConvertHexGridToSquareGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertHexGridToSquareGrid::~ConvertHexGridToSquareGrid() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertHexGridToSquareGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGrid::operator()()
{

  return {};
}
