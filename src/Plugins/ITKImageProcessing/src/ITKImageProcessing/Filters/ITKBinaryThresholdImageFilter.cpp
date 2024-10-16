#include "ITKBinaryThresholdImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkBinaryThresholdImageFilter.h>

using namespace nx::core;

namespace cxITKBinaryThresholdImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

struct ITKBinaryThresholdImageFunctor
{
  float64 lowerThreshold = 0.0;
  float64 upperThreshold = 255.0;
  uint8 insideValue = 1u;
  uint8 outsideValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::BinaryThresholdImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetLowerThreshold(lowerThreshold);
    filter->SetUpperThreshold(upperThreshold);
    filter->SetInsideValue(insideValue);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKBinaryThresholdImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKBinaryThresholdImageFilter::name() const
{
  return FilterTraits<ITKBinaryThresholdImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryThresholdImageFilter::className() const
{
  return FilterTraits<ITKBinaryThresholdImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryThresholdImageFilter::uuid() const
{
  return FilterTraits<ITKBinaryThresholdImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryThresholdImageFilter::humanName() const
{
  return "ITK Binary Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryThresholdImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBinaryThresholdImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryThresholdImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_LowerThreshold_Key, "Lower Threshold", "The lower threshold that a pixel value could be and still be considered 'Inside Value'", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_UpperThreshold_Key, "Upper Threshold",
                                                   "Set the thresholds. The default lower threshold is NumericTraits<InputPixelType>::NonpositiveMin() . The default upper threshold is "
                                                   "NumericTraits<InputPixelType>::max . An exception is thrown if the lower threshold is greater than the upper threshold.",
                                                   255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "Inside Value", "Set the 'inside' pixel value. The default value NumericTraits<OutputPixelType>::max()", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "Outside Value", "Set the 'outside' pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() .", 0u));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKBinaryThresholdImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryThresholdImageFilter::clone() const
{
  return std::make_unique<ITKBinaryThresholdImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryThresholdImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto lowerThreshold = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto upperThreshold = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKBinaryThresholdImageFilter::ArrayOptionsType, cxITKBinaryThresholdImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryThresholdImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto lowerThreshold = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto upperThreshold = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  const cxITKBinaryThresholdImageFilter::ITKBinaryThresholdImageFunctor itkFunctor = {lowerThreshold, upperThreshold, insideValue, outsideValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKBinaryThresholdImageFilter::ArrayOptionsType, cxITKBinaryThresholdImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                            itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_LowerThresholdKey = "LowerThreshold";
constexpr StringLiteral k_UpperThresholdKey = "UpperThreshold";
constexpr StringLiteral k_InsideValueKey = "InsideValue";
constexpr StringLiteral k_OutsideValueKey = "OutsideValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKBinaryThresholdImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKBinaryThresholdImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_LowerThresholdKey, k_LowerThreshold_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_UpperThresholdKey, k_UpperThreshold_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint8>>(args, json, SIMPL::k_InsideValueKey, k_InsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint8>>(args, json, SIMPL::k_OutsideValueKey, k_OutsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
