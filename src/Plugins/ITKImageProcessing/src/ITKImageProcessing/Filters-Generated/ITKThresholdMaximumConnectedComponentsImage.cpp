#include "ITKThresholdMaximumConnectedComponentsImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkThresholdMaximumConnectedComponentsImageFilter.h>

using namespace complex;

namespace cxITKThresholdMaximumConnectedComponentsImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = uint8;

struct ITKThresholdMaximumConnectedComponentsImageFunctor
{
  uint32 minimumObjectSizeInPixels = 0u;
  float64 upperBoundary = std::numeric_limits<double>::max();
  uint8 insideValue = 1u;
  uint8 outsideValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ThresholdMaximumConnectedComponentsImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetMinimumObjectSizeInPixels(minimumObjectSizeInPixels);
    filter->SetUpperBoundary(upperBoundary);
    filter->SetInsideValue(insideValue);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKThresholdMaximumConnectedComponentsImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImage::name() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImage::className() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKThresholdMaximumConnectedComponentsImage::uuid() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImage::humanName() const
{
  return "ITK Threshold Maximum Connected Components Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKThresholdMaximumConnectedComponentsImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKThresholdMaximumConnectedComponentsImage", "ITKConnectedComponents", "ConnectedComponents"};
}

//------------------------------------------------------------------------------
Parameters ITKThresholdMaximumConnectedComponentsImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(k_MinimumObjectSizeInPixels_Key, "MinimumObjectSizeInPixels", "", 0u));
  params.insert(std::make_unique<Float64Parameter>(k_UpperBoundary_Key, "UpperBoundary", "", std::numeric_limits<double>::max()));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "InsideValue", "", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "OutsideValue", "", 0u));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKThresholdMaximumConnectedComponentsImage::clone() const
{
  return std::make_unique<ITKThresholdMaximumConnectedComponentsImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKThresholdMaximumConnectedComponentsImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto minimumObjectSizeInPixels = filterArgs.value<uint32>(k_MinimumObjectSizeInPixels_Key);
  auto upperBoundary = filterArgs.value<float64>(k_UpperBoundary_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKThresholdMaximumConnectedComponentsImage::ArrayOptionsT, cxITKThresholdMaximumConnectedComponentsImage::FilterOutputT>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKThresholdMaximumConnectedComponentsImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto minimumObjectSizeInPixels = filterArgs.value<uint32>(k_MinimumObjectSizeInPixels_Key);
  auto upperBoundary = filterArgs.value<float64>(k_UpperBoundary_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  cxITKThresholdMaximumConnectedComponentsImage::ITKThresholdMaximumConnectedComponentsImageFunctor itkFunctor = {minimumObjectSizeInPixels, upperBoundary, insideValue, outsideValue};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKThresholdMaximumConnectedComponentsImage::ArrayOptionsT, cxITKThresholdMaximumConnectedComponentsImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                                  outputArrayPath, itkFunctor);
}
} // namespace complex
