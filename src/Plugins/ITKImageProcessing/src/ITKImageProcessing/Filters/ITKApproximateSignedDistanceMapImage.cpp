#include "ITKApproximateSignedDistanceMapImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkApproximateSignedDistanceMapImageFilter.h>

using namespace nx::core;

namespace cxITKApproximateSignedDistanceMapImage
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKApproximateSignedDistanceMapImageFunctor
{
  float64 insideValue = 1u;
  float64 outsideValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ApproximateSignedDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetInsideValue(insideValue);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKApproximateSignedDistanceMapImage

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKApproximateSignedDistanceMapImage::name() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKApproximateSignedDistanceMapImage::className() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKApproximateSignedDistanceMapImage::uuid() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKApproximateSignedDistanceMapImage::humanName() const
{
  return "ITK Approximate Signed Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKApproximateSignedDistanceMapImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKApproximateSignedDistanceMapImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKApproximateSignedDistanceMapImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_InsideValue_Key, "InsideValue", "Set/Get intensity value representing the interior of objects in the mask.", 1u));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "Set/Get intensity value representing non-objects in the mask.", 0u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKApproximateSignedDistanceMapImage::clone() const
{
  return std::make_unique<ITKApproximateSignedDistanceMapImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKApproximateSignedDistanceMapImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto insideValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKApproximateSignedDistanceMapImage::ArrayOptionsType, cxITKApproximateSignedDistanceMapImage::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKApproximateSignedDistanceMapImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto insideValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  const cxITKApproximateSignedDistanceMapImage::ITKApproximateSignedDistanceMapImageFunctor itkFunctor = {insideValue, outsideValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKApproximateSignedDistanceMapImage::ArrayOptionsType, cxITKApproximateSignedDistanceMapImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                          outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
