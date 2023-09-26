#include "ITKBinaryProjectionImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBinaryProjectionImageFilter.h>

using namespace complex;

namespace cxITKBinaryProjectionImage
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
} // namespace cxITKBinaryProjectionImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinaryProjectionImage::name() const
{
  return FilterTraits<ITKBinaryProjectionImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryProjectionImage::className() const
{
  return FilterTraits<ITKBinaryProjectionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryProjectionImage::uuid() const
{
  return FilterTraits<ITKBinaryProjectionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryProjectionImage::humanName() const
{
  return "ITK Binary Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryProjectionImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBinaryProjectionImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryProjectionImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "ProjectionDimension", "The index of the projection dimension", 0u));
  params.insert(std::make_unique<Float64Parameter>(
      k_ForegroundValue_Key, "ForegroundValue",
      "Set the value in the image to consider as 'foreground'. Defaults to maximum value of PixelType. Subclasses may alias this to DilateValue or ErodeValue.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue",
                                                   "Set the value used as 'background'. Any pixel value which is not DilateValue is considered background. BackgroundValue is used for defining "
                                                   "boundary conditions. Defaults to NumericTraits<PixelType>::NonpositiveMin() .",
                                                   0.0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryProjectionImage::clone() const
{
  return std::make_unique<ITKBinaryProjectionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryProjectionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinaryProjectionImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryProjectionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  const IDataArray* inputArray = dataStructure.getDataAs<IDataArray>(selectedInputArray);
  if(inputArray->getDataFormat() != "")
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", selectedInputArray.toString()));
  }

  const cxITKBinaryProjectionImage::ITKBinaryProjectionImageFunctor itkFunctor = {projectionDimension, foregroundValue, backgroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKBinaryProjectionImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
