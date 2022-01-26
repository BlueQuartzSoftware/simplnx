#include "ITKApproximateSignedDistanceMapImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkApproximateSignedDistanceMapImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;
using FilterOutputT = float32;

struct ITKApproximateSignedDistanceMapImageFunctor
{
  float64 insideValue = 1u;
  float64 outsideValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ApproximateSignedDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetInsideValue(insideValue);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace

namespace complex
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
  return "ITK::ApproximateSignedDistanceMapImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKApproximateSignedDistanceMapImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKApproximateSignedDistanceMapImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKApproximateSignedDistanceMapImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_InsideValue_Key, "InsideValue", "", 1u));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "", 0u));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKApproximateSignedDistanceMapImage::clone() const
{
  return std::make_unique<ITKApproximateSignedDistanceMapImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKApproximateSignedDistanceMapImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto insideValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT, FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKApproximateSignedDistanceMapImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto insideValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  ITKApproximateSignedDistanceMapImageFunctor itkFunctor = {insideValue, outsideValue};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKApproximateSignedDistanceMapImageFunctor, ArrayOptionsT, FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
