#include "OtsuMultipleThresholdsImageFilter.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/OtsuImageEngine.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
struct OtsuExecute
{
  DataStructure& Data;
  const DataPath& InputPath;
  const DataPath& OutputPath;
  const IDataArray* MaskPtr;
  const ImageProcessing::OtsuImageOptions& Options;
  const std::atomic_bool& ShouldCancel;
  const IFilter::MessageHandler& MessageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& input = Data.getDataRefAs<DataArray<T>>(InputPath).getDataStoreRef();
    auto& output = Data.getDataRefAs<DataArray<uint8>>(OutputPath).getDataStoreRef();
    if(MaskPtr != nullptr && MaskPtr->getDataType() == DataType::boolean)
    {
      return ImageProcessing::ApplyOtsuImage(input, output, &MaskPtr->getIDataStoreRefAs<AbstractDataStore<bool>>(), Options, ShouldCancel, MessageHandler);
    }
    const auto* maskStorePtr = MaskPtr == nullptr ? nullptr : &MaskPtr->getIDataStoreRefAs<AbstractDataStore<uint8>>();
    return ImageProcessing::ApplyOtsuImage(input, output, maskStorePtr, Options, ShouldCancel, MessageHandler);
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
  params.insert(std::make_unique<ChoicesParameter>(
      k_NonfiniteInputHandling_Key, "Nonfinite Input Handling",
      "Error stops on selected NaN or infinity. Warn and Continue retains NaN/negative infinity in the first histogram bin and positive infinity in the last; classification may be unreliable.", 0,
      ChoicesParameter::Choices{"Error", "Warn and Continue"}));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMaskArray_Key, "Use Mask Array", "Explicitly exclude pixels with a zero/false mask value before range and nonfinite checks.", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Select a scalar bool or uint8 mask with one value per input tuple. Nonzero values select pixels.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<UInt8Parameter>(k_MaskedLabel_Key, "Masked Label",
                                                 "Label for excluded pixels. It must be outside the entire configured class interval; downstream filters must explicitly honor it.", uint8{255}));
  params.linkParameters(k_UseMaskArray_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_UseMaskArray_Key, k_MaskedLabel_Key, true);

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
  return 2;
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
  const uint32 labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  const uint32 lastLabel = labelOffset + static_cast<uint32>(numberOfThresholds);
  if(lastLabel > 255)
  {
    return {.outputActions =
                MakeErrorResult<OutputActions>(-8312, fmt::format("Label Offset ({}) plus Number Of Thresholds ({}) exceeds 255. Reduce either value to keep every class label within uint8.",
                                                                  labelOffset, static_cast<uint32>(numberOfThresholds)))};
  }
  if(filterArgs.value<bool>(k_UseMaskArray_Key))
  {
    const uint32 maskedLabel = filterArgs.value<uint8>(k_MaskedLabel_Key);
    if(maskedLabel >= labelOffset && maskedLabel <= lastLabel)
    {
      return {.outputActions = MakeErrorResult<OutputActions>(
                  -8313, fmt::format("Masked Label ({}) collides with the configured class interval [{}, {}]. Select a Masked Label outside this interval or reduce the class interval.", maskedLabel,
                                     labelOffset, lastLabel))};
    }
    const auto maskPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
    const auto& mask = dataStructure.getDataRefAs<IDataArray>(maskPath);
    const auto& input = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
    if(mask.getNumberOfTuples() != input.getNumberOfTuples())
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-8314, fmt::format("Mask Array '{}' has {} tuples, but input '{}' has {}. Select a scalar mask with one value per input tuple.",
                                                                                 maskPath.toString(), mask.getNumberOfTuples(), selectedInputArray.toString(), input.getNumberOfTuples()))};
    }
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

  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key),
                                                  .NumberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key),
                                                  .LabelOffset = filterArgs.value<uint8>(k_LabelOffset_Key),
                                                  .MaskedLabel = filterArgs.value<uint8>(k_MaskedLabel_Key),
                                                  .WarnOnNonfinite = filterArgs.value<ChoicesParameter::ValueType>(k_NonfiniteInputHandling_Key) == 1,
                                                  .ValleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key),
                                                  .ReturnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key)};
  const IDataArray* maskPtr = filterArgs.value<bool>(k_UseMaskArray_Key) ? &dataStructure.getDataRefAs<IDataArray>(filterArgs.value<DataPath>(k_MaskArrayPath_Key)) : nullptr;
  const OtsuExecute execute{
      .Data = dataStructure, .InputPath = selectedInputArray, .OutputPath = outputArrayPath, .MaskPtr = maskPtr, .Options = options, .ShouldCancel = shouldCancel, .MessageHandler = messageHandler};
  return ImageProcessing::AllNumeric::dispatch(dataStructure.getDataRefAs<IDataArray>(selectedInputArray).getDataType(), execute);
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
