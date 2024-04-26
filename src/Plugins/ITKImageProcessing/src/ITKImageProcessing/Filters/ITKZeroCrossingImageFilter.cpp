#include "ITKZeroCrossingImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <ITKZeroCrossingImageFilter.h>

using namespace nx::core;

namespace cxITKZeroCrossingImageFilter
{
using ArrayOptionsType = ITK::SignedScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

struct ITKZeroCrossingImageFilterFunctor
{
  uint8 foregroundValue = 1u;
  uint8 backgroundValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ZeroCrossingImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetForegroundValue(foregroundValue);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace cxITKZeroCrossingImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKZeroCrossingImageFilter::name() const
{
  return FilterTraits<ITKZeroCrossingImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKZeroCrossingImageFilter::className() const
{
  return FilterTraits<ITKZeroCrossingImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKZeroCrossingImageFilter::uuid() const
{
  return FilterTraits<ITKZeroCrossingImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKZeroCrossingImageFilter::humanName() const
{
  return "ITK Zero Crossing Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKZeroCrossingImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKZeroCrossingImageFilter", "ITKImageFeature", "ImageFeature"};
}

//------------------------------------------------------------------------------
Parameters ITKZeroCrossingImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt8Parameter>(k_ForegroundValue_Key, "Foreground Value", "Set/Get the label value for zero-crossing pixels.", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_BackgroundValue_Key, "Background Value", "Set/Get the label value for non-zero-crossing pixels.", 0u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetSignedScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKZeroCrossingImageFilter::clone() const
{
  return std::make_unique<ITKZeroCrossingImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKZeroCrossingImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto foregroundValue = filterArgs.value<uint8>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<uint8>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKZeroCrossingImageFilter::ArrayOptionsType, cxITKZeroCrossingImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKZeroCrossingImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto foregroundValue = filterArgs.value<uint8>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<uint8>(k_BackgroundValue_Key);

  const cxITKZeroCrossingImageFilter::ITKZeroCrossingImageFilterFunctor itkFunctor = {foregroundValue, backgroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKZeroCrossingImageFilter::ArrayOptionsType, cxITKZeroCrossingImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                      shouldCancel);
}
} // namespace nx::core
