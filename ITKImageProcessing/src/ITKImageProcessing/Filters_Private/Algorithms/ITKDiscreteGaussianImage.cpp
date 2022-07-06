#include "ITKDiscreteGaussianImage.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ITKDiscreteGaussianImage::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKDiscreteGaussianImageInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ITKDiscreteGaussianImage::~ITKDiscreteGaussianImage() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ITKDiscreteGaussianImage::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ITKDiscreteGaussianImage::operator()()
{

  return {};
}
