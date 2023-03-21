#include "ImageContouringFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ImageContouring.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImageContouringFilter::name() const
{
  return FilterTraits<ImageContouringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImageContouringFilter::className() const
{
  return FilterTraits<ImageContouringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ImageContouringFilter::uuid() const
{
  return FilterTraits<ImageContouringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImageContouringFilter::humanName() const
{
  return "Image Contour";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImageContouringFilter::defaultTags() const
{
  return {};
}

//------------------------------------------------------------------------------
Parameters ImageContouringFilter::parameters() const
{
  Parameters params;
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImageContouringFilter::clone() const
{
  return std::make_unique<ImageContouringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImageContouringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImageContouringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  ImageContouringInputValues inputValues;

  return ImageContouring(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
