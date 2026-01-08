#include "InitializeImageGeomCellData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
InitializeImageGeomCellData::InitializeImageGeomCellData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         InitializeImageGeomCellDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InitializeImageGeomCellData::~InitializeImageGeomCellData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> InitializeImageGeomCellData::operator()()
{

  return {};
}
