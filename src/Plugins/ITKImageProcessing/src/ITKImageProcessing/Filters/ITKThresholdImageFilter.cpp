#include "ITKThresholdImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkThresholdImageFilter.h>

using namespace nx::core;

namespace cxITKThresholdImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKThresholdImageFunctor
{
  float64 lower = 0.0;
  float64 upper = 1.0;
  float64 outsideValue = 0.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ThresholdImageFilter<InputImageT>;
    auto filter = FilterType::New();
    filter->SetLower(lower);
    filter->SetUpper(upper);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKThresholdImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKThresholdImageFilter::name() const
{
  return FilterTraits<ITKThresholdImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKThresholdImageFilter::className() const
{
  return FilterTraits<ITKThresholdImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKThresholdImageFilter::uuid() const
{
  return FilterTraits<ITKThresholdImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKThresholdImageFilter::humanName() const
{
  return "ITK Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKThresholdImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKThresholdImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKThresholdImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Lower_Key, "Lower", "Set/Get methods to set the lower threshold.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Upper_Key, "Upper", "Set/Get methods to set the upper threshold.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue",
                                                   "The pixel type must support comparison operators. Set the 'outside' pixel value. The default value NumericTraits<PixelType>::ZeroValue() .", 0.0));

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
IFilter::UniquePointer ITKThresholdImageFilter::clone() const
{
  return std::make_unique<ITKThresholdImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKThresholdImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto lower = filterArgs.value<float64>(k_Lower_Key);
  auto upper = filterArgs.value<float64>(k_Upper_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKThresholdImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKThresholdImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto lower = filterArgs.value<float64>(k_Lower_Key);
  auto upper = filterArgs.value<float64>(k_Upper_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  const cxITKThresholdImageFilter::ITKThresholdImageFunctor itkFunctor = {lower, upper, outsideValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKThresholdImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
