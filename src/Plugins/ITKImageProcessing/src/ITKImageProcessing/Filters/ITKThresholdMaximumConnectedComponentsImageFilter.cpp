#include "ITKThresholdMaximumConnectedComponentsImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <ITKThresholdMaximumConnectedComponentsImageFilter.h>

using namespace nx::core;

namespace cxITKThresholdMaximumConnectedComponentsImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

struct ITKThresholdMaximumConnectedComponentsImageFilterFunctor
{
  uint32 minimumObjectSizeInPixels = 0u;
  float64 upperBoundary = std::numeric_limits<double>::max();
  uint8 insideValue = 1u;
  uint8 outsideValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ThresholdMaximumConnectedComponentsImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetMinimumObjectSizeInPixels(minimumObjectSizeInPixels);
    filter->SetUpperBoundary(static_cast<typename InputImageT::PixelType>(std::min<double>(upperBoundary, itk::NumericTraits<typename InputImageT::PixelType>::max())));
    filter->SetInsideValue(insideValue);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKThresholdMaximumConnectedComponentsImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImageFilter::name() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImageFilter::className() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKThresholdMaximumConnectedComponentsImageFilter::uuid() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImageFilter::humanName() const
{
  return "ITK Threshold Maximum Connected Components Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKThresholdMaximumConnectedComponentsImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKThresholdMaximumConnectedComponentsImageFilter", "ITKConnectedComponents", "ConnectedComponents"};
}

//------------------------------------------------------------------------------
Parameters ITKThresholdMaximumConnectedComponentsImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(
      k_MinimumObjectSizeInPixels_Key, "MinimumObjectSizeInPixels",
      "The pixel type must support comparison operators. Set the minimum pixel area used to count objects on the image. Thus, only objects that have a pixel area greater than the minimum pixel area "
      "will be counted as an object in the optimization portion of this filter. Essentially, it eliminates noise from being counted as an object. The default value is zero.",
      0u));
  params.insert(std::make_unique<Float64Parameter>(
      k_UpperBoundary_Key, "Upper Boundary",
      "The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside "
      "value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside "
      "value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity.",
      std::numeric_limits<double>::max()));
  params.insert(std::make_unique<UInt8Parameter>(
      k_InsideValue_Key, "Inside Value",
      "The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside "
      "value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside "
      "value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity.",
      1u));
  params.insert(std::make_unique<UInt8Parameter>(
      k_OutsideValue_Key, "Outside Value",
      "The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside "
      "value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside "
      "value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity.",
      0u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKThresholdMaximumConnectedComponentsImageFilter::clone() const
{
  return std::make_unique<ITKThresholdMaximumConnectedComponentsImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKThresholdMaximumConnectedComponentsImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto minimumObjectSizeInPixels = filterArgs.value<uint32>(k_MinimumObjectSizeInPixels_Key);
  auto upperBoundary = filterArgs.value<float64>(k_UpperBoundary_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKThresholdMaximumConnectedComponentsImageFilter::ArrayOptionsType, cxITKThresholdMaximumConnectedComponentsImageFilter::FilterOutputType>(dataStructure, selectedInputArray,
                                                                                                                                                                   imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKThresholdMaximumConnectedComponentsImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                        const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto minimumObjectSizeInPixels = filterArgs.value<uint32>(k_MinimumObjectSizeInPixels_Key);
  auto upperBoundary = filterArgs.value<float64>(k_UpperBoundary_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  const cxITKThresholdMaximumConnectedComponentsImageFilter::ITKThresholdMaximumConnectedComponentsImageFilterFunctor itkFunctor = {minimumObjectSizeInPixels, upperBoundary, insideValue,
                                                                                                                                    outsideValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKThresholdMaximumConnectedComponentsImageFilter::ArrayOptionsType, cxITKThresholdMaximumConnectedComponentsImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
