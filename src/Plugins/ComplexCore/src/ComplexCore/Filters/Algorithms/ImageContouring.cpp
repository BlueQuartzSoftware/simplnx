#include "ImageContouring.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImageContouring::ImageContouring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImageContouringInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImageContouring::~ImageContouring() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImageContouring::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImageContouring::operator()()
{
  return {};
}
