#include "ITKOtsuMultipleThresholdsImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkOtsuMultipleThresholdsImageFilter.h>

using namespace nx::core;

namespace cxITKOtsuMultipleThresholdsImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

struct ITKOtsuMultipleThresholdsImageFunctor
{
  uint8 numberOfThresholds = 1u;
  uint8 labelOffset = 0u;
  uint32 numberOfHistogramBins = 128u;
  bool valleyEmphasis = false;
  bool returnBinMidpoint = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::OtsuMultipleThresholdsImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetNumberOfThresholds(numberOfThresholds);
    filter->SetLabelOffset(labelOffset);
    filter->SetNumberOfHistogramBins(numberOfHistogramBins);
    filter->SetValleyEmphasis(valleyEmphasis);
    filter->SetReturnBinMidpoint(returnBinMidpoint);
    return filter;
  }
};
} // namespace cxITKOtsuMultipleThresholdsImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImageFilter::name() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImageFilter::className() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKOtsuMultipleThresholdsImageFilter::uuid() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImageFilter::humanName() const
{
  return "ITK Otsu Multiple Thresholds Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKOtsuMultipleThresholdsImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKOtsuMultipleThresholdsImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKOtsuMultipleThresholdsImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt8Parameter>(k_NumberOfThresholds_Key, "Number Of Thresholds", "Set/Get the number of thresholds. Default is 1.", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_LabelOffset_Key, "Label Offset", "Set/Get the offset which labels have to start from. Default is 0.", 0u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfHistogramBins_Key, "Number Of Histogram Bins", "Set/Get the number of histogram bins. Default is 128.", 128u));
  params.insert(std::make_unique<BoolParameter>(k_ValleyEmphasis_Key, "Valley Emphasis", "Set/Get the use of valley emphasis. Default is false.", false));
  params.insert(
      std::make_unique<BoolParameter>(k_ReturnBinMidpoint_Key, "ReturnBinMidpoint", "Should the threshold value be mid-point of the bin or the maximum? Default is to return bin maximum.", false));

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
IFilter::UniquePointer ITKOtsuMultipleThresholdsImageFilter::clone() const
{
  return std::make_unique<ITKOtsuMultipleThresholdsImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKOtsuMultipleThresholdsImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKOtsuMultipleThresholdsImageFilter::ArrayOptionsType, cxITKOtsuMultipleThresholdsImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKOtsuMultipleThresholdsImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);

  const cxITKOtsuMultipleThresholdsImageFilter::ITKOtsuMultipleThresholdsImageFunctor itkFunctor = {numberOfThresholds, labelOffset, numberOfHistogramBins, valleyEmphasis, returnBinMidpoint};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKOtsuMultipleThresholdsImageFilter::ArrayOptionsType, cxITKOtsuMultipleThresholdsImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                          outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_NumberOfThresholdsKey = "NumberOfThresholds";
constexpr StringLiteral k_LabelOffsetKey = "LabelOffset";
constexpr StringLiteral k_NumberOfHistogramBinsKey = "NumberOfHistogramBins";
constexpr StringLiteral k_ValleyEmphasisKey = "ValleyEmphasis";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKOtsuMultipleThresholdsImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKOtsuMultipleThresholdsImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint8>>(args, json, SIMPL::k_NumberOfThresholdsKey, k_NumberOfThresholds_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint8>>(args, json, SIMPL::k_LabelOffsetKey, k_LabelOffset_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint32>>(args, json, SIMPL::k_NumberOfHistogramBinsKey, k_NumberOfHistogramBins_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ValleyEmphasisKey, k_ValleyEmphasis_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
