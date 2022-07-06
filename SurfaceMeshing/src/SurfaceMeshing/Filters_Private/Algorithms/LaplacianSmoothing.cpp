#include "LaplacianSmoothing.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
LaplacianSmoothing::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LaplacianSmoothingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
LaplacianSmoothing::~LaplacianSmoothing() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& LaplacianSmoothing::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> LaplacianSmoothing::operator()()
{

  return {};
}
