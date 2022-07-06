#include "GenerateFeatureIDsbyBoundingBoxes.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GenerateFeatureIDsbyBoundingBoxes::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               GenerateFeatureIDsbyBoundingBoxesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateFeatureIDsbyBoundingBoxes::~GenerateFeatureIDsbyBoundingBoxes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateFeatureIDsbyBoundingBoxes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateFeatureIDsbyBoundingBoxes::operator()()
{

  return {};
}
