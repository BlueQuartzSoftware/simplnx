#include "ITKBinaryProjectionImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkBinaryProjectionImageFilter.h>

using namespace nx::core;

namespace cxITKBinaryProjectionImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKBinaryProjectionImageFunctor
{
  uint32 projectionDimension = 0u;
  float64 foregroundValue = 1.0;
  float64 backgroundValue = 0.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::BinaryProjectionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetProjectionDimension(projectionDimension);
    filter->SetForegroundValue(foregroundValue);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace cxITKBinaryProjectionImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKBinaryProjectionImageFilter::name() const
{
  return FilterTraits<ITKBinaryProjectionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryProjectionImageFilter::className() const
{
  return FilterTraits<ITKBinaryProjectionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryProjectionImageFilter::uuid() const
{
  return FilterTraits<ITKBinaryProjectionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryProjectionImageFilter::humanName() const
{
  return "ITK Binary Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryProjectionImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBinaryProjectionImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryProjectionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "ProjectionDimension", "", 0u));
  params.insert(std::make_unique<Float64Parameter>(
      k_ForegroundValue_Key, "ForegroundValue",
      "Set the value in the image to consider as 'foreground'. Defaults to maximum value of PixelType. Subclasses may alias this to DilateValue or ErodeValue.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue",
                                                   "Set the value used as 'background'. Any pixel value which is not DilateValue is considered background. BackgroundValue is used for defining "
                                                   "boundary conditions. Defaults to NumericTraits<PixelType>::NonpositiveMin() .",
                                                   0.0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Array Name",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryProjectionImageFilter::clone() const
{
  return std::make_unique<ITKBinaryProjectionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryProjectionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinaryProjectionImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryProjectionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  const cxITKBinaryProjectionImageFilter::ITKBinaryProjectionImageFunctor itkFunctor = {projectionDimension, foregroundValue, backgroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKBinaryProjectionImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
