#include "ITKApproximateSignedDistanceMapImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkApproximateSignedDistanceMapImageFilter.h>

using namespace nx::core;

namespace cxITKApproximateSignedDistanceMapImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKApproximateSignedDistanceMapImageFilterFunctor
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
} // namespace cxITKApproximateSignedDistanceMapImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKApproximateSignedDistanceMapImageFilter::name() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKApproximateSignedDistanceMapImageFilter::className() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKApproximateSignedDistanceMapImageFilter::uuid() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKApproximateSignedDistanceMapImageFilter::humanName() const
{
  return "ITK Approximate Signed Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKApproximateSignedDistanceMapImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKApproximateSignedDistanceMapImageFilter", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKApproximateSignedDistanceMapImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_InsideValue_Key, "Inside Value", "Set/Get intensity value representing the interior of objects in the mask.", 1u));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "Outside Value", "Set/Get intensity value representing non-objects in the mask.", 0u));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKApproximateSignedDistanceMapImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKApproximateSignedDistanceMapImageFilter::clone() const
{
  return std::make_unique<ITKApproximateSignedDistanceMapImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKApproximateSignedDistanceMapImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto insideValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKApproximateSignedDistanceMapImageFilter::ArrayOptionsType, cxITKApproximateSignedDistanceMapImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKApproximateSignedDistanceMapImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto insideValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  const cxITKApproximateSignedDistanceMapImageFilter::ITKApproximateSignedDistanceMapImageFilterFunctor itkFunctor = {insideValue, outsideValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKApproximateSignedDistanceMapImageFilter::ArrayOptionsType, cxITKApproximateSignedDistanceMapImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                                      outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
