#include "OtsuMultipleThresholdsImageFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <vector>

using namespace nx::core;

namespace
{
struct OtsuOperation
{
  uint32 numBins = 128u;
  uint8 numThresholds = 1u;
  uint8 labelOffset = 0u;
  bool valleyEmphasis = false;
  bool returnBinMidpoint = false;

  template <class T, class U>
  auto makeClassifyOp(const std::vector<float64>& thresholds) const
  {
    const uint8 offset = labelOffset;
    return [thresholds, offset](T value) -> U {
      // class index = count of thresholds this value STRICTLY EXCEEDS, + labelOffset. std::lower_bound gives the
      // count of thresholds < value, i.e. a value exactly equal to a threshold lands in the LOWER class -- which
      // is ITK's ThresholdLabeler convention ("values equal to a threshold are in the lower class"). Using
      // std::upper_bound here put a value == threshold one class too high (e.g. every voxel of a constant image).
      const auto idx = static_cast<usize>(std::lower_bound(thresholds.begin(), thresholds.end(), static_cast<float64>(value)) - thresholds.begin());
      return static_cast<U>(offset + idx);
    };
  }
};
} // namespace

namespace nx::core
{
std::string OtsuMultipleThresholdsImageFilter::name() const
{
  return FilterTraits<OtsuMultipleThresholdsImageFilter>::name;
}
std::string OtsuMultipleThresholdsImageFilter::className() const
{
  return FilterTraits<OtsuMultipleThresholdsImageFilter>::className;
}
Uuid OtsuMultipleThresholdsImageFilter::uuid() const
{
  return FilterTraits<OtsuMultipleThresholdsImageFilter>::uuid;
}
std::string OtsuMultipleThresholdsImageFilter::humanName() const
{
  return "Otsu Multiple Thresholds Image Filter";
}
std::vector<std::string> OtsuMultipleThresholdsImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "OtsuMultipleThresholds", "Thresholding"};
}
Parameters OtsuMultipleThresholdsImageFilter::parameters() const
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
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}
IFilter::VersionType OtsuMultipleThresholdsImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer OtsuMultipleThresholdsImageFilter::clone() const
{
  return std::make_unique<OtsuMultipleThresholdsImageFilter>();
}
IFilter::PreflightResult OtsuMultipleThresholdsImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // Data-independent guard: an out-of-range threshold count makes the backend return an empty threshold
  // set, which would silently classify every voxel into a single (labelOffset-valued) class.
  if(numberOfThresholds == 0 || numberOfThresholds >= numberOfHistogramBins)
  {
    return {
        MakeErrorResult<OutputActions>(-8311, fmt::format("NumberOfThresholds ({}) must be >= 1 and < NumberOfHistogramBins ({}).", static_cast<uint32>(numberOfThresholds), numberOfHistogramBins))};
  }

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(-8310, fmt::format("Otsu Multiple Thresholds requires a single-component (scalar) input array, but '{}' has {} components.", selectedInputArray.toString(),
                                                              inputArray.getNumberOfComponents()))};
  }

  // Output is a uint8 label image regardless of the (numeric) input element type.
  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysUInt8>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> OtsuMultipleThresholdsImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);

  const OtsuOperation operation{numberOfHistogramBins, numberOfThresholds, labelOffset, valleyEmphasis, returnBinMidpoint};
  return ImageProcessing::ExecuteHistogramImageFilter<ImageProcessing::AllNumeric>(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
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

Result<Arguments> OtsuMultipleThresholdsImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = OtsuMultipleThresholdsImageFilter().getDefaultArguments();

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
